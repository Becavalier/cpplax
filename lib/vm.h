#ifndef	_VM_H
#define	_VM_H

#define DEBUG_TRACE_EXECUTION
#define STACK_MAX 65536

#include <iostream>
#include <array>
#include <variant>
#include <unordered_map>
#include <cassert>
#include "./chunk.h"
#include "./type.h"
#include "./compiler.h"
#include "./error.h"

struct VM {
  using typeVMStack = std::array<typeRuntimeValue, STACK_MAX>;
  const Chunk& chunk;
  typeVMStack stack;
  typeVMCodeArray::const_iterator ip;  // Points to the instruction about to be executed.
  typeVMStack::iterator stackTop;  // Points just past the last used element.
  HeapObj* objs;  // Points to the head of the heap object list.
  InternedConstants* internedConstants;
  std::unordered_map<HeapObj*, typeRuntimeValue> globals;
  VM(const Chunk& chunk, HeapObj* objs, InternedConstants* internedConstants) : chunk(chunk), ip(chunk.code.cbegin()), stackTop(stack.begin()), objs(objs), internedConstants(internedConstants) {}
  auto currentLine(void) const {
    return chunk.getLine(ip - 1);
  }
  auto top(void) {
    return stackTop - 1;
  }
  void push(typeRuntimeValue v) {
    *stackTop = v;
    ++stackTop;
  }
  auto peek(size_t distance) const {
    return *(stackTop - 1 - distance);
  }
  auto pop(void) {
    --stackTop;
    return *stackTop;
  }
  void resetStack(void) {
    stackTop = stack.data();
  }
  void checkNumberOperands(int8_t n) const {
    while (--n >= 0) {
      if (!std::holds_alternative<typeRuntimeNumericValue>(peek(n)))
        throw VMError { currentLine(), n > 1 ? "operand must be a number." : "operands must be numbers." };
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
    return *ip++;
  }
  auto readShort(void) {
    ip += 2;
    return static_cast<uint16_t>(*(ip - 2) << 8 | *(ip - 1));
  }
  auto readConstant(void) {
    return chunk.constants[readByte()];
  }
  auto readConstantOfExpectedType(HeapObjType type) {
    const auto name = std::get<HeapObj*>(readConstant());
    assert(name->type == type);  // Runtime assertion.
    return name;
  }
  void testDefinedVariable(HeapObj* name, const std::string& errorMsg) {
    if (!globals.contains(name)) throw VMError { currentLine(), errorMsg };
  }
  auto getDefinedVariable(HeapObj* name, const std::string& errorMsg) {
    const auto value = globals.find(name);
    if (value == globals.end()) throw VMError { currentLine(), errorMsg };
    return value;
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
        ChunkDebugger::printValue(*it);
        printf(" ] ");
      }
      printf("<-\n");
      ChunkDebugger::disassembleInstruction(chunk, ip);
#endif
      const auto instruction = readByte();
      switch (instruction) {
        case OpCode::OP_ADD: {
          const auto y = pop();
          const auto x = pop();
          if (std::holds_alternative<HeapObj*>(x) && std::holds_alternative<HeapObj*>(y)) {
            const auto heapX = std::get<HeapObj*>(x);
            const auto heapY = std::get<HeapObj*>(y);
            if (heapX->type == HeapObjType::OBJ_STRING && heapY->type == HeapObjType::OBJ_STRING) {
              push(internedConstants->add(heapX->toStringObj()->str + heapY->toStringObj()->str, &objs));
              break;
            }
          } else {
            push(std::get<typeRuntimeNumericValue>(x) +  std::get<typeRuntimeNumericValue>(y));
            break;
          }
          throw VMError { currentLine(), "operands must be two numbers or two strings." };
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
          return VMResult::INTERPRET_OK;
        }
        case OpCode::OP_CONSTANT: {
          auto constant = readConstant();
          push(constant);
          break;
        }
        case OpCode::OP_NIL: push(std::monostate {}); break;
        case OpCode::OP_TRUE: push(true); break;
        case OpCode::OP_FALSE: push(false); break;
        case OpCode::OP_NOT: push(isFalsey(pop())); break;
        case OpCode::OP_EQUAL: {
          const auto x = pop();
          const auto y = pop();
          if (std::holds_alternative<HeapObj*>(x) && std::holds_alternative<HeapObj*>(y)) {
            const auto heapX = std::get<HeapObj*>(x);
            const auto heapY = std::get<HeapObj*>(y);
            if (heapX->type == heapY->type) {
              switch (heapX->type) {
                case HeapObjType::OBJ_STRING: {
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
          globals[readConstantOfExpectedType(HeapObjType::OBJ_STRING)] = pop();
          break;
        }
        case OpCode::OP_GET_GLOBAL: {
          const auto name = readConstantOfExpectedType(HeapObjType::OBJ_STRING);
          const auto value = getDefinedVariable(name, "Undefined variable '" + name->toStringObj()->str + "'.");
          push(value->second);
          break;
        }
        case OpCode::OP_SET_GLOBAL: {
          const auto name = readConstantOfExpectedType(HeapObjType::OBJ_STRING);
          testDefinedVariable(name, "Undefined variable '" + name->toStringObj()->str + "'.");
          globals[name] = peek(0);  // Assignment expression doesn’t pop the value off the stack.
          break;
        }
        case OpCode::OP_GET_LOCAL: {
          push(stack[readByte()]);  // Take the operand from stack (local slot), and load the value.
          break;
        }
        case OpCode::OP_SET_LOCAL: {
          stack[readByte()] = peek(0);
          break;
        }
        case OpCode::OP_JUMP_IF_FALSE: {
          auto offset = readShort();
          if (isFalsey(peek(0))) ip += offset;
          break;
        }
        case OpCode::OP_LOOP: {
          ip -= readShort();
          break;
        }
        case OpCode::OP_JUMP: {
          ip += readShort();
          break;
        }
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
      return VMResult::INTERPRET_RUNTIME_ERROR;
    }
  }
};

#endif
