#ifndef	_CONSTANT_H
#define	_CONSTANT_H

/**
 * Constant pool (mainly for interned strings).
*/

#include <string_view>
#include <unordered_map>
#include "./object.h"

struct Obj;
class InternedConstants  {
  std::unordered_map<std::string_view, Obj*> internedConstants;
 public:
  InternedConstants() = default;
  Obj* add(std::string_view str, Obj** objs) {
    const auto target = internedConstants.find(str);
    if (target != internedConstants.end()) {
      return target->second;  // Reuse the existing interned string obj.
    } else {
      const auto heapStr = new StringObj { str, objs };
      internedConstants[heapStr->str] = heapStr;
      return heapStr;  // Generate a new sting obj on the heap.
    }
  };
};

#endif
