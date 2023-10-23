#include "./memory.h"
#include "./object.h"
#include "./vm.h"
#include "./common.h"
#include "./constant.h"

void Memory::free(bool leaveStr) {  // Freeing non-ObjString objects first.
  auto obj = objs;
  Obj* prev = nullptr;
  while (obj != nullptr) {
    if (leaveStr && obj->type == ObjType::OBJ_STRING) {
      prev = obj;
      obj = obj->next;        
      continue;
    }
    auto next = obj->next;
    freeObj(obj);
    if (obj == objs) objs = next; 
    if (prev != nullptr) prev->next = next;
    obj = next;
  }
  if (leaveStr) free(false);
}

void Memory::markObject(Obj* obj) {
  if (obj == nullptr || obj->isMarked) return;
#ifdef DEBUG_LOG_GC
  printf("-- [%p] marked ", obj);
  printValue(obj);
  std::cout << " --" << std::endl;
#endif
  obj->isMarked = true;
  vm->grayStack.push_back(obj);  // Keeping track of all of the gray objects.
}

void Memory::markValue(typeRuntimeValue& value) {
  if (std::holds_alternative<Obj*>(value)) {
    markObject(std::get<Obj*>(value));
  }
}

template<typename T> 
void Memory::markTable(typeVMStore<T>& map) {
  for (auto& [key, value] : map) {
    markObject(key);
    if constexpr (std::is_same_v<T, Obj*>) {
      markObject(value);
    } else {
      markValue(value); 
    }
  }
}

void Memory::markArray(typeRuntimeConstantArray& array) {
  for (auto& v : array) {
    markValue(v);
  }
} 

void Memory::markCompilerRoots(Compiler* rootCompiler) {
  auto currCompiler = rootCompiler;
  while (currCompiler != nullptr) {
    markObject(currCompiler->compilingFunc);
    currCompiler = currCompiler->enclosing;
  }
}

void Memory::markRoots(void) {
  // Marking any objects that the VM can reach directly without going through a reference in some other object.
  for (auto slot = vm->stack.begin(); slot < vm->stackTop; slot++) {
    markValue(*slot);
  }
  for (size_t i = 0; i < vm->frameCount; i++) {
    markObject(vm->frames[i].frameEntity);  // Mark "ObjClosure" or "FuncObj".
  }
  for (Obj* upvalue = vm->openUpvalues; upvalue != nullptr; upvalue = upvalue->next) {
    markObject(upvalue);
  }
  markTable(vm->globals);
  markCompilerRoots(compiler);
  markObject(vm->initString);
}

void Memory::blackenObject(Obj* obj) {
#ifdef DEBUG_LOG_GC
  printf("-- [%p] Blacken ", obj);
  printValue(obj);
  std::cout << " --" << std::endl;
#endif
  // A black object is any object whose isMarked field is set and that is no longer in the gray stack.
  switch (obj->type) {
    case ObjType::OBJ_NATIVE:
    case ObjType::OBJ_STRING:
      break;
    case ObjType::OBJ_UPVALUE: {  
      markValue(obj->cast<ObjUpvalue>()->closed);
      break;
    }
    case ObjType::OBJ_FUNCTION: {
      auto function = retrieveObjFunc(obj);
      markObject(function->name);
      markArray(function->chunk.constants);
      break;
    }
    case ObjType::OBJ_CLOSURE: {
      auto closure = obj->cast<ObjClosure>();
      markObject(closure->function);
      for (const auto& uvObj : closure->upvalues) {
        markObject(uvObj);
      }
      break;
    }
    case ObjType::OBJ_CLASS: {
      auto klass = obj->cast<ObjClass>();
      markObject(klass->name);
      markTable(klass->methods);
      break;
    }
    case ObjType::OBJ_INSTANCE: {
      auto instance = obj->cast<ObjInstance>();
      markObject(instance->klass);
      markTable(instance->fields);
      break;
    }
    case ObjType::OBJ_BOUND_METHOD: {
      auto bound = obj->cast<ObjBoundMethod>();
      markValue(bound->receiver);
      markObject(bound->method);
      break;
    }
  }
}

void Memory::traceReferences(void) {
  while (vm->grayStack.size() > 0) {
    auto obj = vm->grayStack.back();
    vm->grayStack.pop_back();
    blackenObject(obj);
  }
}

void Memory::sweep(void) {
  Obj* prev = nullptr;
  auto obj = objs;
  while (obj != nullptr) {
    if (obj->isMarked) {
      obj->isMarked = false;
      prev = obj;
      obj = obj->next;
    } else {
      auto unreached = obj;
      obj = obj->next;
      // Skip the "white" object.
      if (prev != nullptr) {
        prev->next = obj;
      } else {
        objs = obj;
      }
      freeObj(unreached);
    }
  } 
}

void Memory::tableRemoveWhite(void) {
  // Removing the weak reference from the constant table.
  auto& table = vm->internedConstants.table;
  for (auto it = table.begin(); it != table.end();) {
    if (!it->first.empty() && !it->second->isMarked) {
      it = table.erase(it);
    } else {
      it++;
    }
  }
}

void Memory::gc(void) {
  if (vm == nullptr || compiler == nullptr) return;
#ifdef DEBUG_LOG_GC
  std::cout << "\n-- GC BEGIN --" << std::endl;
  const auto before = bytesAllocated;
#endif
  markRoots();
  traceReferences();
  tableRemoveWhite();
  sweep();
  nextGC = bytesAllocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
  std::cout << "-- GC END --\n" << std::endl;
  printf("Collected %zu bytes (from %zu to %zu) next at %zu.\n", before - bytesAllocated, before, bytesAllocated, nextGC);
#endif
}
