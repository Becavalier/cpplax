#ifndef	_OBJECT_H
#define	_OBJECT_H

/**
 * Object structure.
*/

#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "./chunk.h" 
#include "./type.h"

struct StringObj;
struct FuncObj;
struct NativeObj;
struct Obj {
  ObjType type;
  Obj* next;  // Make up an intrusive list.
  explicit Obj(ObjType type, Obj* next = nullptr) : type(type), next(next) {}
  StringObj* toStringObj(void);
  FuncObj* toFuncObj(void);
  NativeObj* toNativeObj(void);
  virtual ~Obj() {};
};

struct StringObj : public Obj {
  std::string str;
  StringObj(std::string_view str, Obj** next) : Obj(ObjType::OBJ_STRING, *next), str(str) {
    *next = this;
  }
  ~StringObj() {}
};

struct FuncObj : public Obj {
  uint8_t arity;
  Chunk chunk;
  StringObj* name;
  FuncObj() : Obj(ObjType::OBJ_FUNCTION), arity(0), name(nullptr) {}
  ~FuncObj() {}
};

struct NativeObj : public Obj {
  using typeNativeFn = typeRuntimeValue (*)(uint8_t, typeVMStack::const_iterator);
  uint8_t arity;
  typeNativeFn function;
  StringObj* name;
  NativeObj(typeNativeFn function, uint8_t arity, StringObj* name) : Obj(ObjType::OBJ_NATIVE), arity(arity), function(function), name(name) {}
  ~NativeObj() {}
};

#endif
