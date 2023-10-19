#ifndef	_VM_H
#define	_VM_H

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

struct Memory;
struct VM {
  Memory* mem;
  size_t frameCount;  // Store the number of ongoing function calls.
  typeVMStack stack;
  typeVMFrames frames;
  typeVMStack::iterator stackTop;  // Points to the element that just past the last used element.
  InternedConstants internedConstants { mem };
  typeVMStore globals;
  UpvalueObj* openUpvalues = nullptr;
  CallFrame* currentFrame;
  // For GC.
  std::vector<Obj*> grayStack = {};
  bool isStatusOk = true;
  explicit VM(std::vector<Token>& tokens, Memory* mem) : mem(mem), frameCount(0), stackTop(stack.begin()) {
    // Compiling into byte codes, it returns a new "FuncObj" containing the compiled top-level code. 
    const auto function = Compiler { tokens.cbegin(), mem, &internedConstants }.compile();
    if (!Error::hadError) {
      tokens.clear();
      initVM(function);
    } else {
      isStatusOk = false;
    }
  }
  VM(const VM&) = delete;
  VM(const VM&&) = delete;
  void initVM(FuncObj*);
  size_t currentLine(void) {
    return retrieveFuncObj(currentFrame->frameEntity)->chunk.getLine(currentFrame->ip - 1);
  }
  auto top(void) const {
    return stackTop - 1;
  }
  void push(const typeRuntimeValue& v) {
    *stackTop = v;
    ++stackTop;
  }
  auto& peek(size_t distance = 0) const {
    return *(stackTop - 1 - distance);
  }
  auto& pop(void) {
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
  auto readByte(void) {
    return *currentFrame->ip++;
  }
  auto readShort(void) {
    currentFrame->ip += 2;
    return static_cast<uint16_t>(*(currentFrame->ip - 2) << 8 | *(currentFrame->ip - 1));
  }
  auto& readConstant(void) {
    return retrieveFuncObj(currentFrame->frameEntity)->chunk.constants[readByte()];
  }
  auto readConstantOfExpectedType(ObjType type) {
    const auto name = std::get<Obj*>(readConstant());
    assert(name->type == type);  // Runtime assertion.
    return name;
  }
  void call(Obj*, uint8_t);
  void callValue(typeRuntimeValue, uint8_t);
  void defineNative(const char*, NativeObj::typeNativeFn, uint8_t);
  UpvalueObj* captureUpvalue(typeRuntimeValue*);
  void closeUpvalues(typeRuntimeValue*);
  VMResult run(void);
  void stackTrace(void);
  void freeVM(void);
  VMResult interpret(void);
};

#endif
