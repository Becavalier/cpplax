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
  bool isMarked = false;
  template<typename T> static const char* printObjNameByType(void);
  static std::string printObjNameByEnum(Obj*);
  template<typename T>
  T* cast(void) {
    return static_cast<T*>(this);
  }
  explicit Obj(ObjType type, Obj* next = nullptr) : type(type), next(next) {}
  virtual ~Obj() {};
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

struct ClassObj : public Obj {
  StringObj* name;
  typeVMStore<Obj*> methods;
  ClassObj(Obj** next, StringObj* name) : Obj(ObjType::OBJ_CLASS, *next), name(name) {
    *next = this;
  }
  ~ClassObj() {}
};

struct InstanceObj : public Obj {
  ClassObj* klass;
  typeVMStore<> fields = {};
  InstanceObj(Obj** next, ClassObj* klass) : Obj(ObjType::OBJ_INSTANCE, *next), klass(klass) {
    *next = this;
  }
  ~InstanceObj() {}
};

struct BoundMethodObj : public Obj {
  typeRuntimeValue receiver;   // "InstanceObj*".
  Obj* method;
  BoundMethodObj(Obj** next, typeRuntimeValue& receiver, Obj* method) : Obj(ObjType::OBJ_BOUND_METHOD, *next), receiver(receiver), method(method) {
    *next = this;
  }
  ~BoundMethodObj() {}
};  

// Helper functions.
inline auto retrieveFuncObj(Obj* obj) { 
  return obj->type == ObjType::OBJ_FUNCTION ? static_cast<FuncObj*>(obj) : obj->cast<ClosureObj>()->function;  // Falling through to "ClosureObj".
}

template<typename T>
const char* Obj::printObjNameByType(void) {
  using K = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<K, FuncObj>) return "FuncObj";
  else if constexpr (std::is_same_v<K, NativeObj>) return "NativeObj";
  else if constexpr (std::is_same_v<K, ClosureObj>) return "ClosureObj";
  else if constexpr (std::is_same_v<K, StringObj>) return "StringObj";
  else if constexpr (std::is_same_v<K, UpvalueObj>) return "UpvalueObj";
  else if constexpr (std::is_same_v<K, ClassObj>) return "ClassObj";
  else if constexpr (std::is_same_v<K, InstanceObj>) return "InstanceObj";
  else if constexpr (std::is_same_v<K, BoundMethodObj>) return "BoundMethodObj";
  return "Unknown Type";
}

#endif
