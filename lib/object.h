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
struct Obj {
  ObjType type;
  Obj* next;  // Make up an intrusive list.
  explicit Obj(ObjType type, Obj* next = nullptr) : type(type), next(next) {}
  StringObj* toStringObj(void);
  FuncObj* toFuncObj(void);
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

#endif
