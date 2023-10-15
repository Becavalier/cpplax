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
#include "./chunk.h" 
#include "./type.h"

struct Obj {
  ObjType type;
  Obj* next;  // Make up an intrusive list.
  explicit Obj(ObjType type, Obj* next = nullptr) : type(type), next(next) {}
  virtual ~Obj() {};
};

struct StringObj : public Obj {
  std::string str;
  StringObj(std::string_view str, Obj** next) : Obj(ObjType::OBJ_STRING, *next), str(str) {
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
  FuncObj() : Obj(ObjType::OBJ_FUNCTION), arity(0), upvalueCount(0), name(nullptr) {}
  ~FuncObj() {}
};


struct UpvalueObj : public Obj {
  typeRuntimeValue* location;  // Pointing to the value on the stack.
  typeRuntimeValue closed = std::monostate {};
  UpvalueObj* next;
  explicit UpvalueObj(typeRuntimeValue* location, UpvalueObj* next = nullptr) : Obj(ObjType::OBJ_UPVALUE), location(location), next(next) {}
  ~UpvalueObj() {}
}; 

// The wrapper of "FuncObj" which includes runtime state for the variables the function closes over.
struct ClosureObj : public Obj {
  FuncObj* function;
  std::vector<UpvalueObj*> upvalues;  // Each closure has an array of upvalues.
  uint32_t upvalueCount;
  explicit ClosureObj(FuncObj* functionObj) : 
    Obj(ObjType::OBJ_CLOSURE), 
    function(functionObj), 
    upvalues(function->upvalueCount, nullptr), 
    upvalueCount(function->upvalueCount) {}
  ~ClosureObj() {
    upvalues.clear();
  }
};

struct NativeObj : public Obj {
  using typeNativeFn = typeRuntimeValue (*)(uint8_t, typeVMStack::const_iterator);
  uint8_t arity;
  typeNativeFn function;
  StringObj* name;
  NativeObj(typeNativeFn function, uint8_t arity, StringObj* name) : Obj(ObjType::OBJ_NATIVE), arity(arity), function(function), name(name) {}
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
