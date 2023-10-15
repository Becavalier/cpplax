#ifndef	_OBJECT_H
#define	_OBJECT_H

/**
 * Object structure.
*/
#include <cassert>
#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cstdio>
#include "./chunk.h" 
#include "./type.h"

struct Obj {
  ObjType type;
  Obj* next;  // Make up an intrusive list.
  explicit Obj(ObjType type, Obj* next = nullptr) : type(type), next(next) {}
  virtual ~Obj() {
#ifdef DEBUG_LOG_GC
    printf("%p free type %hhu\n", this, type);
#endif
  };
};

struct StringObj : public Obj {
  std::string str;
  StringObj(Obj** next, std::string_view str) : Obj(ObjType::OBJ_STRING, *next), str(str) {
    *next = this;
  }
  ~StringObj() {}
};

// The “raw” compile-time state of a function declaration.
struct FuncObj : public Obj {
  uint8_t arity;
  uint32_t upvalueCount;
  Chunk chunk;
  StringObj* name;
  explicit FuncObj(Obj** next) : Obj(ObjType::OBJ_FUNCTION, *next), arity(0), upvalueCount(0), name(nullptr) {
    *next = this;
  }
  ~FuncObj() {}
};

struct UpvalueObj : public Obj {
  typeRuntimeValue* location;  // Pointing to the value on the stack.
  typeRuntimeValue closed = std::monostate {};
  UpvalueObj* nextValue;
  UpvalueObj(
    Obj** next, 
    typeRuntimeValue* location, 
    UpvalueObj* nextValue = nullptr) : 
    Obj(ObjType::OBJ_UPVALUE, *next), 
    location(location), 
    nextValue(nextValue) {
      *next = this;
    }
  ~UpvalueObj() {}
}; 

// The wrapper of "FuncObj" which includes runtime state for the variables the function closes over.
struct ClosureObj : public Obj {
  FuncObj* function;
  std::vector<UpvalueObj*> upvalues;  // Each closure has an array of upvalues.
  uint32_t upvalueCount;
  explicit ClosureObj(Obj** next, FuncObj* functionObj) : 
    Obj(ObjType::OBJ_CLOSURE, *next), 
    function(functionObj), 
    upvalues(function->upvalueCount, nullptr), 
    upvalueCount(function->upvalueCount) {
      *next = this;
    }
  ~ClosureObj() {}
};

struct NativeObj : public Obj {
  using typeNativeFn = typeRuntimeValue (*)(uint8_t, typeVMStack::const_iterator);
  uint8_t arity;
  typeNativeFn function;
  StringObj* name;
  NativeObj(
    Obj** next,
    typeNativeFn function,
    uint8_t arity, 
    StringObj* name) : 
    Obj(ObjType::OBJ_NATIVE, *next),
    arity(arity), 
    function(function), 
    name(name) {
      *next = this;
    }
  ~NativeObj() {}
};


// Helper functions.
inline auto castStringObj(Obj* obj) {
  return static_cast<StringObj*>(obj);
}

inline auto castClosureObj(Obj* obj) {
  return static_cast<ClosureObj*>(obj);
}

inline auto retrieveFuncObj(Obj* obj) { 
  return obj->type == ObjType::OBJ_FUNCTION 
    ? static_cast<FuncObj*>(obj) 
    : castClosureObj(obj)->function;   // Falling through to "ClosureObj".
}

inline auto castNativeObj(Obj* obj) {
  return static_cast<NativeObj*>(obj);
}

#endif
