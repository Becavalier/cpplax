#ifndef	_VM_H
#define	_VM_H

#include <iostream>
#include <cstdio>
#include <array>
#include <variant>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include "./common.h"
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
  typeVMStore<> globals;
  CallFrame* currentFrame;
  ObjUpvalue* openUpvalues = nullptr;
  Obj* initString = nullptr;
  // For GC.
  std::vector<Obj*> grayStack = {};
  bool isStatusOk = true;
  explicit VM(std::vector<Token>& tokens, Memory* mem) : mem(mem), frameCount(0), stackTop(stack.begin()) {
    // Compiling into byte codes, it returns a new "ObjFunc" containing the compiled top-level code. 
    const auto function = Compiler { tokens, tokens.cbegin(), mem, &internedConstants }.compile();
    if (!Error::hadError) {
      tokens.clear();
      initVM(function);
    } else {
      isStatusOk = false;
    }
  }
  VM(const VM&) = delete;
  VM(const VM&&) = delete;
  void initVM(ObjFunc*);
  size_t currentLine(void) {
    return retrieveObjFunc(currentFrame->frameEntity)->chunk.getLine(currentFrame->ip - 1);
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
  template<typename T>
  T& peekOfType(size_t distance = 0) const {
    return std::get<T>(peek(distance));
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
  void checkNumberOperands(uint8_t n) {
    auto counter = n + 1;
    while (--counter >= 1) {
      if (!std::holds_alternative<typeRuntimeNumericValue>(peek(counter - 1)))
        throwRuntimeError(n == 1 ? "operand must be a number." : "operands must be numbers.");
    }
  }
  auto isFalsey(const typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return true;
    if (std::holds_alternative<bool>(obj)) return !std::get<bool>(obj);
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
    return retrieveObjFunc(currentFrame->frameEntity)->chunk.constants[readByte()];
  }
  template<typename T>
  T& readConstantOfType(void) {
    return std::get<T>(readConstant());
  }
  void call(Obj*, uint8_t);
  void callValue(typeRuntimeValue&, uint8_t);
  void defineNative(const char*, ObjNative::typeNativeFn, uint8_t);
  ObjUpvalue* captureUpvalue(typeRuntimeValue*);
  void closeUpvalues(typeRuntimeValue*);
  VMResult run(void);
  void stackTrace(void);
  void freeVM(void);
  VMResult interpret(void);
  void defineMethod(Obj*);
  void bindMethod(ObjClass*, Obj*);
  void invokeFromClass(ObjClass*, Obj*, uint8_t);
  void invoke(Obj*, uint8_t);
};

#endif
