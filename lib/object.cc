#include "./object.h"

StringObj* Obj::toStringObj(void) {
  return static_cast<StringObj*>(this);
}

FuncObj* Obj::toFuncObj(void) {
  return static_cast<FuncObj*>(this);
}

NativeObj* Obj::toNativeObj(void) {
  return static_cast<NativeObj*>(this);
}
