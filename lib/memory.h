#ifndef	_MEMORY_H
#define	_MEMORY_H

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <type_traits>
#include <unordered_map>
#include "./common.h"
#include "./object.h"
#include "./type.h"
#include "./helper.h" 

struct VM;
struct Compiler;
struct Memory {
  Obj* objs = nullptr;  // Points to the head of the heap object list.
  VM* vm = nullptr;
  Compiler* compiler = nullptr;
  size_t bytesAllocated = 0;
  size_t nextGC = 1024 * 1024;
  Memory() = default;
  void setVM(VM* vmPtr) { vm = vmPtr; }
  void setCompiler(Compiler* compilerPtr) { compiler = compilerPtr; }
  inline void freeObj(Obj* obj) {
#ifdef DEBUG_LOG_GC
    printf("[%p] Free type %s\n", obj, obj->toString().c_str());
#endif
    bytesAllocated -= sizeof(*obj);
    delete obj;
  }
  template<typename T, typename ...Args> 
  inline T* makeObj(Args && ...args) {
#ifdef DEBUG_STRESS_GC
    gc();
#else
    if ((bytesAllocated += sizeof(T)) > nextGC) {
      gc();
    }
#endif
    const auto& obj = new T { &objs, std::forward<Args>(args)... };
#ifdef DEBUG_LOG_GC
    printf("\n-- [%p] Allocate %zu bytes for '", obj, sizeof(T));      
    std::cout << Obj::printObjNameByType<T>() << "' --\n" << std::endl;
#endif
    return obj;
  }
  void markRoots(void);
  void gc(void);
  void free(bool = true);
  void markObject(Obj*);
  void markValue(typeRuntimeValue&);
  template<typename T> void markTable(typeVMStore<T>&);
  void markCompilerRoots(Compiler*);
  void markArray(typeRuntimeConstantArray&);
  void traceReferences(void);
  void blackenObject(Obj*);
  void sweep(void);
  void tableRemoveWhite(void);
};

#endif
