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

struct ObjString : public Obj {
  std::string str;
  ObjString(Obj** next, std::string_view str) : Obj(ObjType::OBJ_STRING, *next), str(str) {
    *next = this;
  }
  ~ObjString() {}
};

// The “raw” compile-time state of a function declaration.
struct ObjFunc : public Obj {
  uint8_t arity;
  uint32_t upvalueCount;
  Chunk chunk;
  ObjString* name;
  explicit ObjFunc(Obj** next) : Obj(ObjType::OBJ_FUNCTION, *next), arity(0), upvalueCount(0), name(nullptr) {
    *next = this;
  }
  ~ObjFunc() {}
};

struct ObjUpvalue : public Obj {
  typeRuntimeValue* location;  // Pointing to the value on the stack.
  typeRuntimeValue closed = std::monostate {};
  ObjUpvalue* nextValue;
  ObjUpvalue(
    Obj** next, 
    typeRuntimeValue* location, 
    ObjUpvalue* nextValue = nullptr) : 
    Obj(ObjType::OBJ_UPVALUE, *next), 
    location(location), 
    nextValue(nextValue) {
      *next = this;
    }
  ~ObjUpvalue() {}
}; 

// The wrapper of "ObjFunc" which includes runtime state for the variables the function closes over.
struct ObjClosure : public Obj {
  ObjFunc* function;
  std::vector<ObjUpvalue*> upvalues;  // Each closure has an array of upvalues.
  uint32_t upvalueCount;
  explicit ObjClosure(Obj** next, ObjFunc* functionObj) : 
    Obj(ObjType::OBJ_CLOSURE, *next), 
    function(functionObj), 
    upvalues(function->upvalueCount, nullptr), 
    upvalueCount(function->upvalueCount) {
      *next = this;
    }
  ~ObjClosure() {}
};

struct ObjNative : public Obj {
  using typeNativeFn = typeRuntimeValue (*)(uint8_t, typeVMStack::const_iterator);
  uint8_t arity;
  typeNativeFn function;
  Obj* name;
  ObjNative(
    Obj** next,
    typeNativeFn function,
    uint8_t arity, 
    Obj* name) : 
    Obj(ObjType::OBJ_NATIVE, *next),
    arity(arity), 
    function(function), 
    name(name) {
      *next = this;
    }
  ~ObjNative() {}
};

struct ObjClass : public Obj {
  Obj* name;
  typeVMStore<Obj*> methods;
  ObjClass(Obj** next, Obj* name) : Obj(ObjType::OBJ_CLASS, *next), name(name) {
    *next = this;
  }
  ~ObjClass() {}
};

struct ObjInstance : public Obj {
  ObjClass* klass;
  typeVMStore<> fields = {};
  ObjInstance(Obj** next, ObjClass* klass) : Obj(ObjType::OBJ_INSTANCE, *next), klass(klass) {
    *next = this;
  }
  ~ObjInstance() {}
};

struct ObjBoundMethod : public Obj {
  typeRuntimeValue receiver;   // "ObjInstance*".
  Obj* method;
  ObjBoundMethod(Obj** next, typeRuntimeValue& receiver, Obj* method) : Obj(ObjType::OBJ_BOUND_METHOD, *next), receiver(receiver), method(method) {
    *next = this;
  }
  ~ObjBoundMethod() {}
};  

// Helper functions.
inline auto retrieveObjFunc(Obj* obj) { 
  return obj->type == ObjType::OBJ_FUNCTION ? static_cast<ObjFunc*>(obj) : obj->cast<ObjClosure>()->function;  // Falling through to "ObjClosure".
}

template<typename T>
const char* Obj::printObjNameByType(void) {
  using K = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<K, ObjFunc>) return "ObjFunc";
  else if constexpr (std::is_same_v<K, ObjNative>) return "ObjNative";
  else if constexpr (std::is_same_v<K, ObjClosure>) return "ObjClosure";
  else if constexpr (std::is_same_v<K, ObjString>) return "ObjString";
  else if constexpr (std::is_same_v<K, ObjUpvalue>) return "ObjUpvalue";
  else if constexpr (std::is_same_v<K, ObjClass>) return "ObjClass";
  else if constexpr (std::is_same_v<K, ObjInstance>) return "ObjInstance";
  else if constexpr (std::is_same_v<K, ObjBoundMethod>) return "ObjBoundMethod";
  return "Unknown Type";
}

#endif
