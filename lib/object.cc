#include "./object.h"

inline auto printFunction(ObjFunc* func) {
  return "<fn " + (func->name == nullptr ? "script" : func->name->str) + ">";
};

std::string Obj::printObjNameByEnum(Obj* obj) {
  switch (obj->type) {
    case ObjType::OBJ_STRING: return static_cast<ObjString*>(obj)->str.data();
    case ObjType::OBJ_FUNCTION: return printFunction(retrieveObjFunc(obj));
    case ObjType::OBJ_NATIVE: return "<fn native>";
    case ObjType::OBJ_CLOSURE: return printFunction(obj->cast<ObjClosure>()->function);
    case ObjType::OBJ_UPVALUE: return "<upvalue>";
    case ObjType::OBJ_CLASS: return "<class " + obj->cast<ObjClass>()->name->cast<ObjString>()->str + ">"; 
    case ObjType::OBJ_INSTANCE: return "<instance " + obj->cast<ObjInstance>()->klass->name->cast<ObjString>()->str + ">";
    case ObjType::OBJ_BOUND_METHOD: return printFunction(retrieveObjFunc(obj->cast<ObjBoundMethod>()->method));
    default: return "<internal>";
  }
}