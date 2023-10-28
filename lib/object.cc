#include "./object.h"
#include "./helper.h"

std::string ObjUpvalue::toString(void) {
  return stringifyVariantValue(std::holds_alternative<std::monostate>(closed) ? *location : closed);
}
