#ifndef	_MEMORY_H
#define	_MEMORY_H

#include <iostream>
#include <cstdlib>
#include "./macro.h"
#include "./object.h"
#include "./type.h"

// #define ALLOCATE(type, count) \
//     (type*)reallocate(NULL, 0, sizeof(type) * (count))

// void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
//   if (newSize == 0) {
//     free(pointer);
//     return NULL;
//   }

//   void* result = realloc(pointer, newSize);
//   return result;
// }


struct Memory {
  Obj* objs = nullptr;  // Points to the head of the heap object list.
  template<typename T, typename ...Args>
  inline T* makeObj(Args && ...args) {
    return new T { &objs, std::forward<Args>(args)... };
  }
  static void collectGarbage(void) {
#ifdef DEBUG_LOG_GC
    std::cout << "-- GC BEGIN --" << std::endl;
#endif


#ifdef DEBUG_LOG_GC
    std::cout << "-- GC END --" << std::endl;
#endif
  }
};

void* operator new(size_t size) {
  #ifdef DEBUG_STRESS_GC
    Memory::collectGarbage();  // Collecting right before allocation.
  #endif
  return malloc(size);
}

#endif
