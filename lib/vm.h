#ifndef	_VM_H
#define	_VM_H

#define DEBUG_TRACE_EXECUTION
#define STACK_MAX 65536

#include <iostream>
#include <array>
#include "./chunk.h"
#include "./type.h"
#include "./compiler.h"

struct VM {
  using typeVMStack = std::array<typeVMValue, STACK_MAX>;
  const Chunk& chunk;
  typeVMStack stack;
  typeVMCodeArray::const_iterator ip;  // Points to the instruction about to be executed.
  typeVMStack::iterator stackTop;  // Points just past the last used element.
  explicit VM(const Chunk& chunk) : chunk(chunk), ip(chunk.code.cbegin()), stackTop(stack.begin()) {}
  ~VM() {}
  auto top(void) {
    return stackTop - 1;
  }
  void push(typeVMValue v) {
    *stackTop = v;
    ++stackTop;
  }
  typeVMValue pop(void) {
    --stackTop;
    return *stackTop;
  }
  void resetStack(void) {
    stackTop = stack.data();
  }
  VMResult run(void) {
    #define READ_BYTE() (*ip++)
    #define READ_CONSTANT() (chunk.constants[READ_BYTE()])
    #define BINARY_OP(op) \
      do { \
        auto b = pop();  /* The left operand would be at the bottom. */ \
        auto a = pop(); \
        push(a op b); \
      } while (false)
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
      printf("          ");
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
        case OpCode::OP_ADD: BINARY_OP(+); break;
        case OpCode::OP_SUBTRACT: BINARY_OP(-); break;
        case OpCode::OP_MULTIPLY: BINARY_OP(*); break;
        case OpCode::OP_DIVIDE: BINARY_OP(/); break;
        case OpCode::OP_NEGATE: {
          *top() = -*top();
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
      }
    }
    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
  }
  VMResult interpret(void) {
    return run();
  }
};

#endif
