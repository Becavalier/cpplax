#ifndef	_VM_H
#define	_VM_H

#define DEBUG_TRACE_EXECUTION
#define STACK_MAX 65536

#include <iostream>
#include <array>
#include <variant>
#include "./chunk.h"
#include "./type.h"
#include "./compiler.h"

struct VM {
  using typeVMStack = std::array<typeRuntimeValue, STACK_MAX>;
  const Chunk& chunk;
  typeVMStack stack;
  typeVMCodeArray::const_iterator ip;  // Points to the instruction about to be executed.
  typeVMStack::iterator stackTop;  // Points just past the last used element.
  HeapObj* objs;  // Points to the head of the heap object list.
  VM(const Chunk& chunk, HeapObj* objs) : chunk(chunk), ip(chunk.code.cbegin()), stackTop(stack.begin()), objs(objs) {}
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
  void freeVM(void) {
    freeObjects();
  }
  VMResult run(void) {
    #define READ_BYTE() (*ip++)
    #define READ_CONSTANT() (chunk.constants[READ_BYTE()])
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
      const auto instruction = READ_BYTE();
      switch (instruction) {
        case OpCode::OP_ADD: {
          const auto y = pop();
          const auto x = pop();
          if (std::holds_alternative<HeapObj*>(x) && std::holds_alternative<HeapObj*>(y)) {
            const auto heapX = std::get<HeapObj*>(x);
            const auto heapY = std::get<HeapObj*>(y);
            if (heapX->type == HeapObjType::OBJ_STRING && heapY->type == HeapObjType::OBJ_STRING) {
              push(new HeapStringObj { static_cast<HeapStringObj*>(heapX)->str + static_cast<HeapStringObj*>(heapY)->str, &objs });
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
          ChunkDebugger::printValue(pop());
          printf("\n");
          return VMResult::INTERPRET_OK;
        }
        case OpCode::OP_CONSTANT: {
          auto constant = READ_CONSTANT();
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
                  push(static_cast<HeapStringObj*>(heapX)->str == static_cast<HeapStringObj*>(heapY)->str);
                  break;
                }
                default: ;
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
      }
    }
    #undef READ_BYTE
    #undef READ_CONSTANT
  }
  VMResult interpret(void) {
    try {
      const auto result = run();
      freeVM();
      return result;
    } catch(const VMError* err) {
      return VMResult::INTERPRET_RUNTIME_ERROR;
    }
  }
};

#endif
