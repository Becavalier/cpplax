#ifndef	_VM_H
#define	_VM_H

#define DEBUG_TRACE_EXECUTION

#include <iostream>
#include <cstdio>
#include <array>
#include <variant>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include "./macro.h"
#include "./chunk.h"
#include "./type.h"
#include "./compiler.h"
#include "./error.h"
#include "./constant.h"
#include "./object.h"
#include "./native.h"

/**
 * Representing a single ongoing function call, -
 * it tracks where on the stack the callee function’s locals begin, and where the caller should resume.
*/
struct CallFrame {
  Obj* frameEntity;
  typeVMCodeArray::const_iterator ip;
  typeVMStack::iterator slots;  // The starting position on the stack of each calling function.
};

using typeVMFrames = std::array<CallFrame, FRAMES_MAX>;

struct VM {
  size_t frameCount;  // Store the number of ongoing function calls.
  typeVMStack stack;
  typeVMFrames frames;
  typeVMStack::iterator stackTop;  // Points to the element that just past the last used element.
  Obj* objs;  // Points to the head of the heap object list.
  InternedConstants* internedConstants;
  std::unordered_map<Obj*, typeRuntimeValue> globals;
  UpvalueObj* openUpvalues = nullptr;
  CallFrame* currentFrame;
  VM(FuncObj* function, Obj* objs, InternedConstants* internedConstants) : frameCount(0), stackTop(stack.begin()), objs(objs), internedConstants(internedConstants) {
    defineNative("print", printNative, 1);
    defineNative("clock", clockNative, 0);
    push(function);  // Save the top-level function onto the stack.
    call(function, 0);  // Add a frame for the calling function.
  }
  VM(const VM&) = delete;
  VM(const VM&&) = delete;
  auto currentLine(void) {
    return retrieveFuncObj(currentFrame->frameEntity)->chunk.getLine(currentFrame->ip - 1);
  }
  auto top(void) const {
    return stackTop - 1;
  }
  void push(typeRuntimeValue v) {
    *stackTop = v;
    ++stackTop;
  }
  auto peek(size_t distance) const {
    return *(stackTop - 1 - distance);
  }
  typeRuntimeValue& pop(void) {
    --stackTop;
    return *stackTop;
  }
  void resetStack(void) {
    stackTop = stack.data();
    frameCount = 0;
  }
  void throwRuntimeError(const std::string& msg) {
    throw VMError { currentLine(), msg };
  }
  void checkNumberOperands(int8_t n) {
    while (--n >= 0) {
      if (!std::holds_alternative<typeRuntimeNumericValue>(peek(n)))
        throwRuntimeError(n > 1 ? "operand must be a number." : "operands must be numbers.");
    }
  }
  auto isFalsey(const typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return true;
    if (std::holds_alternative<bool>(obj)) return !std::get<bool>(obj);
    if (std::holds_alternative<std::string_view>(obj)) return std::get<std::string_view>(obj).size() == 0;
    if (std::holds_alternative<typeRuntimeNumericValue>(obj)) return isDoubleEqual(std::get<typeRuntimeNumericValue>(obj), 0);
    return false;
  }
  void freeObjects(void) {
    auto obj = objs;
    while (obj != nullptr) {
      auto next = obj->next;
      delete obj;
      obj = next;
    }
  }
  void freeInternedConstants(void) {
    delete internedConstants;
  }
  void freeVM(void) {
    freeObjects();
    freeInternedConstants();
  }
  auto readByte(void) {
    return *currentFrame->ip++;
  }
  auto readShort(void) {
    currentFrame->ip += 2;
    return static_cast<uint16_t>(*(currentFrame->ip - 2) << 8 | *(currentFrame->ip - 1));
  }
  auto readConstant(void) {
    return retrieveFuncObj(currentFrame->frameEntity)->chunk.constants[readByte()];
  }
  auto readConstantOfExpectedType(ObjType type) {
    const auto name = std::get<Obj*>(readConstant());
    assert(name->type == type);  // Runtime assertion.
    return name;
  }
  void testDefinedVariable(Obj* name, const std::string& errorMsg) {
    if (!globals.contains(name)) throwRuntimeError(errorMsg);
  }
  auto getDefinedVariable(Obj* name, const std::string& errorMsg) {
    const auto value = globals.find(name);
    if (value == globals.end()) throwRuntimeError(errorMsg);
    return value;
  }
  void call(Obj* obj, uint8_t argCount) {
    const auto function = retrieveFuncObj(obj);
    if (argCount != function->arity) {
      throwRuntimeError((std::ostringstream {} << "expected " << +function->arity << " arguments but got " << +argCount << ".").str());
    }
    if (frameCount == FRAMES_MAX) {
      throwRuntimeError("stack overflow.");
    }
    currentFrame = &frames[frameCount++];
    currentFrame->frameEntity = obj;
    currentFrame->ip = function->chunk.code.cbegin();
    currentFrame->slots = stackTop - argCount - 1;
  } 
  void callValue(typeRuntimeValue callee, uint8_t argCount) {
    if (std::holds_alternative<Obj*>(callee)) {
      const auto calleeObj = std::get<Obj*>(callee);
      switch (calleeObj->type) {
        case ObjType::OBJ_NATIVE: {
          const auto nativeFunc = castNativeObj(calleeObj);
          if (nativeFunc->arity != argCount) {
            throwRuntimeError("incorrect number of arguments passed to native function '" + nativeFunc->name->str + "'." );
          }
          const auto result = nativeFunc->function(argCount, stackTop - argCount);
          stackTop -= argCount + 1;
          push(result);
          return;
        }
        default: {
          call(calleeObj, argCount);  // OBJ_FUNCTION, OBJ_CLOSURE.
          return;
        };
      }
    }
    throwRuntimeError("can only call functions and classes.");
  }
  void defineNative(const char* name, NativeObj::typeNativeFn function, uint8_t arity) {
    /**
     * The "push" and "pop" instructions here are for GC. 
     * Storing them on the value stack makes sure the objects won't be collected during initialization.
    */
    push(internedConstants->add(name, &objs));
    const auto obj = std::get<Obj*>(stack[DEFAULT_IDX]);
    push(new NativeObj { function, arity, castStringObj(obj), });
    globals[obj] = stack[DEFAULT_IDX + 1];
    pop();
    pop();
  }
  UpvalueObj* captureUpvalue(typeRuntimeValue* local) {
    UpvalueObj* prevUpvalue = nullptr;
    auto upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {  // Searching from stack top to bottom.
      prevUpvalue = upvalue;
      upvalue = upvalue->next;
    }
    if (upvalue != nullptr && upvalue->location == local) {
      return upvalue;
    }
    const auto createdUpvalue = new UpvalueObj { local, upvalue };
    if (prevUpvalue == nullptr) {
      openUpvalues = createdUpvalue;  // Empty list.
    } else {
      prevUpvalue->next = createdUpvalue;  // Insert into the list at a right position.
    }
    return createdUpvalue;
  }
  void closeUpvalues(typeRuntimeValue* last) {
    while (openUpvalues != nullptr && openUpvalues->location >= last) {
      auto upvalue = openUpvalues;
      upvalue->closed = *upvalue->location;  // Save closed upvalue onto the heap "UpvalueObj" object.
      upvalue->location = &upvalue->closed;
      openUpvalues = upvalue->next;
    }
  }
  VMResult run(void) {
    #define NUM_BINARY_OP(op) \
      do { \
        checkNumberOperands(2); \
        auto b = std::get<typeRuntimeNumericValue>(pop());  /* The left operand would be at the bottom. */ \
        auto a = std::get<typeRuntimeNumericValue>(pop()); \
        push(a op b); \
      } while (false)
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
      printf("          ■ ");
      for (auto it = stack.cbegin(); it < stackTop; it++) {
        printf("[ ");
        printValue(*it);
        printf(" ] ");
      }
      printf("<-\n");
      auto currentIp = currentFrame->ip;
      ChunkDebugger::disassembleInstruction(retrieveFuncObj(currentFrame->frameEntity)->chunk, currentIp);
#endif
      const auto instruction = readByte();
      switch (instruction) {
        case OpCode::OP_ADD: {
          const auto y = pop();
          const auto x = pop();
          if (std::holds_alternative<Obj*>(x) && std::holds_alternative<Obj*>(y)) {
            const auto heapX = std::get<Obj*>(x);
            const auto heapY = std::get<Obj*>(y);
            if (heapX->type == ObjType::OBJ_STRING && heapY->type == ObjType::OBJ_STRING) {
              push(internedConstants->add(castStringObj(heapX)->str + castStringObj(heapY)->str, &objs));
              break;
            }
          } else {
            push(std::get<typeRuntimeNumericValue>(x) +  std::get<typeRuntimeNumericValue>(y));
            break;
          }
          throwRuntimeError("operands must be two numbers or two strings.");
        }
        case OpCode::OP_SUBTRACT: NUM_BINARY_OP(-); break;
        case OpCode::OP_MULTIPLY: NUM_BINARY_OP(*); break;
        case OpCode::OP_DIVIDE: NUM_BINARY_OP(/); break;
        case OpCode::OP_NEGATE: {
          checkNumberOperands(1);
          *top() = -std::get<typeRuntimeNumericValue>(*top());
          break;
        }
        case OpCode::OP_RETURN: {
          const auto result = pop();
          closeUpvalues(currentFrame->slots);
          frameCount--;
          if (frameCount == 0) {
            pop();  // Dicard the main script function.
            return VMResult::INTERPRET_OK;
          }
          stackTop = currentFrame->slots;  // Discard all the unused locals.
          push(result);
          currentFrame = &frames[frameCount - 1];
          break;
        }
        case OpCode::OP_CONSTANT: {
          push(readConstant());
          break;
        }
        case OpCode::OP_NIL: push(std::monostate {}); break;
        case OpCode::OP_TRUE: push(true); break;
        case OpCode::OP_FALSE: push(false); break;
        case OpCode::OP_NOT: push(isFalsey(pop())); break;
        case OpCode::OP_EQUAL: {
          const auto x = pop();
          const auto y = pop();
          if (std::holds_alternative<Obj*>(x) && std::holds_alternative<Obj*>(y)) {
            const auto heapX = std::get<Obj*>(x);
            const auto heapY = std::get<Obj*>(y);
            if (heapX->type == heapY->type) {
              switch (heapX->type) {
                case ObjType::OBJ_STRING: {
                  push(heapX == heapY);
                  break;
                }
                default: {
                  push(false); 
                  break;
                }
              }
            } else {
              push(false);
            }
          } else {
            push(x == y);
          }
          break;
        }
        case OpCode::OP_GREATER: NUM_BINARY_OP(>); break;
        case OpCode::OP_LESS: NUM_BINARY_OP(<); break;
        case OpCode::OP_POP: pop(); break;
        case OpCode::OP_DEFINE_GLOBAL: {
          globals[readConstantOfExpectedType(ObjType::OBJ_STRING)] = pop();
          break;
        }
        case OpCode::OP_GET_GLOBAL: {
          const auto name = readConstantOfExpectedType(ObjType::OBJ_STRING);
          const auto value = getDefinedVariable(name, "undefined variable '" + castStringObj(name)->str + "'.");
          push(value->second);
          break;
        }
        case OpCode::OP_SET_GLOBAL: {
          const auto name = readConstantOfExpectedType(ObjType::OBJ_STRING);
          testDefinedVariable(name, "undefined variable '" + castStringObj(name)->str + "'.");
          globals[name] = peek(0);  // Assignment expression doesn’t pop the value off the stack.
          break;
        }
        case OpCode::OP_GET_LOCAL: {
          push(*(currentFrame->slots + readByte()));  // Take the operand from stack (local slot), and load the value.
          break;
        }
        case OpCode::OP_SET_LOCAL: {
          *(currentFrame->slots + readByte()) = peek(0);
          break;
        }
        case OpCode::OP_JUMP_IF_FALSE: {
          auto offset = readShort();
          if (isFalsey(peek(0))) currentFrame->ip += offset;
          break;
        }
        case OpCode::OP_LOOP: {
          currentFrame->ip -= readShort();
          break;
        }
        case OpCode::OP_JUMP: {
          currentFrame->ip += readShort();
          break;
        }
        case OpCode::OP_CALL: {
          /**
                      frame->slots           StackTop
                            │                    │
                            ▼                    ▼
              0──────1──────2──────3──────4──────5
              │script│  10  │ fn A │ arg1 │ arg2 │
              └──────┴──────0──────1──────2──────3
                            ┼   A() CallFrame    ┼
          */
          const auto argCount = readByte();
          callValue(peek(argCount), argCount);
          break;
        }
        case OpCode::OP_GET_UPVALUE: {
          const auto slot = readByte();
          push(*castClosureObj(currentFrame->frameEntity)->upvalues[slot]->location);
          break;
        }
        case OpCode::OP_SET_UPVALUE: {
          const auto slot = readByte();
          *castClosureObj(currentFrame->frameEntity)->upvalues[slot]->location = peek(0);
          break;
        }
        case OpCode::OP_CLOSURE: {
          auto closure = new ClosureObj { retrieveFuncObj(std::get<Obj*>(readConstant())) };
          push(closure);
          for (uint32_t i = 0; i < closure->upvalueCount; i++) {
            uint8_t isLocal = readByte();
            uint8_t index = readByte();
            if (isLocal == 1) {
              closure->upvalues[i] = captureUpvalue(&*(currentFrame->slots + index));
            } else {
              closure->upvalues[i] = castClosureObj(currentFrame->frameEntity)->upvalues[index];
            }
          }
          break;
        }
        case OpCode::OP_CLOSE_UPVALUE: {
          closeUpvalues(&*(stackTop - 1));  // Hoisting the local to heap.
          pop();  // Then, discard it from the stack.
          break;
        }
      }
    }
  }
  void stackTrace(void) {
    for (auto i = frameCount; i >= 1; i--) {
      auto frame = &frames[i - 1];
      auto function = retrieveFuncObj(frame->frameEntity);
      auto instruction = frame->ip - function->chunk.code.cbegin() - 1;
      fprintf(stderr, "[Line %zu] in ", function->chunk.getLine(instruction));
      if (function->name == NULL) {
        fprintf(stderr, "script.\n");
      } else {
        fprintf(stderr, "%s().\n", function->name->str.c_str());
      }
    }
  }
  VMResult interpret(void) {
    try {
      const auto result = run();
      freeVM();
      return result;
    } catch(const VMError& err) {
      Error::vmError(err);
      stackTrace();
      return VMResult::INTERPRET_RUNTIME_ERROR;
    }
  }
};

CallFrame* currentFrame = nullptr;

#endif
