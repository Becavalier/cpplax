#ifndef	_CONSTANT_H
#define	_CONSTANT_H

/**
 * Constant pool (mainly for interned strings).
*/

#include <string_view>
#include <unordered_map>
#include "./object.h"
#include "./memory.h"

struct Obj;
struct InternedConstants  {
  Memory* mem;
  std::unordered_map<std::string_view, Obj*> table;
  explicit InternedConstants(Memory* memPtr) : mem(memPtr) {};
  Obj* add(std::string_view str) {
    const auto target = table.find(str);
    if (target != table.end()) {
      return target->second;  // Reuse the existing interned string obj.
    } else {
      const auto heapStr = mem->makeObj<StringObj>(str);
      table[str] = heapStr;
      return heapStr;  // Generate a new sting obj on the heap.
    }
  };
};

#endif
