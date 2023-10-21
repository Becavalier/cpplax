#include "./object.h"

inline auto printFunction(FuncObj* func) {
  return "<fn " + (func->name == nullptr ? "script" : func->name->str) + ">";
};

std::string Obj::printObjNameByEnum(Obj* obj) {
  switch (obj->type) {
    case ObjType::OBJ_STRING: return static_cast<StringObj*>(obj)->str.data();
    case ObjType::OBJ_FUNCTION: return printFunction(retrieveFuncObj(obj));
    case ObjType::OBJ_NATIVE: return "<fn native>";
    case ObjType::OBJ_CLOSURE: return printFunction(obj->cast<ClosureObj>()->function);
    case ObjType::OBJ_UPVALUE: return "<upvalue>";
    case ObjType::OBJ_CLASS: return "<class " + obj->cast<ClassObj>()->name->str + ">"; 
    case ObjType::OBJ_INSTANCE: return "<instance " + obj->cast<InstanceObj>()->klass->name->str + ">";
    case ObjType::OBJ_BOUND_METHOD: return printFunction(retrieveFuncObj(obj->cast<BoundMethodObj>()->method));
    default: return "<internal>";
  }
}