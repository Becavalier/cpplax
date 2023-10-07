#include "./type.h"

HeapStringObj* HeapObj::toStringObj(void) {
  return static_cast<HeapStringObj*>(this);
}
