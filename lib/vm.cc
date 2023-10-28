#include <string>
#include "./common.h"
#include "./vm.h"
#include "./native.h"
#include "./memory.h"
#include "./object.h"

void VM::initVM(ObjFunc* function) {
  mem->setVM(this);
  initString = internedConstants.add(INITIALIZER_NAME);
  defineNative("print", nativePrint, 1);
  defineNative("clock", nativeClock, 0);
  push(function);  // Save the top-level function onto the stack.
  call(function, 0);  // Add a frame for the calling function.
}

void VM::freeVM(void) {
  initString = nullptr;
  mem->free();
} 

void VM::defineNative(const char* name, ObjNative::typeNativeFn function, uint8_t arity) {
  const auto nativeName = internedConstants.add(name);
  globals[nativeName] = nullptr;  // For comforting GC.
  globals[nativeName] = mem->makeObj<ObjNative>(function, arity, nativeName);
}

ObjUpvalue* VM::captureUpvalue(typeRuntimeValue* local) {
  ObjUpvalue* prevUpvalue = nullptr;
  auto upvalue = openUpvalues;
  while (upvalue != nullptr && upvalue->location > local) {  // Searching from stack top to bottom.
    prevUpvalue = upvalue;
    upvalue = upvalue->nextValue;
  }
  if (upvalue != nullptr && upvalue->location == local) {
    return upvalue;
  }
  const auto createdUpvalue = mem->makeObj<ObjUpvalue>(local, upvalue);
  if (prevUpvalue == nullptr) {
    openUpvalues = createdUpvalue;  // Empty list.
  } else {
    prevUpvalue->nextValue = createdUpvalue;  // Insert into the list at a right position.
  }
  return createdUpvalue;
}

void VM::closeUpvalues(typeRuntimeValue* last) {
  while (openUpvalues != nullptr && openUpvalues->location >= last) {
    auto upvalue = openUpvalues;
    upvalue->closed = *upvalue->location;  // Save closed upvalue onto the heap "ObjUpvalue" object.
    upvalue->location = &upvalue->closed;
    openUpvalues = upvalue->nextValue;
  }
}

void VM::call(Obj* obj, uint8_t argCount) {
  const auto function = retrieveObjFunc(obj);
  if (argCount != function->arity) {
    throwRuntimeError((std::ostringstream {} << "expected " << +function->arity << " arguments but got " << +argCount << ".").str());
  }
  if (frameCount == FRAMES_MAX) {
    throwRuntimeError("stack overflow.");
  }
  currentFrame = &frames[frameCount++];
  currentFrame->frameEntity = obj;
  currentFrame->ip = function->chunk.code.cbegin();
  currentFrame->slots = stackTop - argCount - 1;
}

void VM::callValue(typeRuntimeValue& callee, uint8_t argCount) {
  if (std::holds_alternative<Obj*>(callee)) {
    const auto calleeObj = std::get<Obj*>(callee);
    switch (calleeObj->type) {
      case ObjType::OBJ_NATIVE: {
        const auto nativeFunc = calleeObj->cast<ObjNative>();;
        if (nativeFunc->arity != argCount) {
          throwRuntimeError("incorrect number of arguments passed to native function '" + nativeFunc->name->cast<ObjString>()->str + "'.");
        }
        const auto result = nativeFunc->function(argCount, stackTop - argCount);
        stackTop -= argCount + 1;
        push(result);
        return;
      }
      case ObjType::OBJ_CLASS: {
        const auto klass = calleeObj->cast<ObjClass>();
        *(stackTop - argCount - 1) = mem->makeObj<ObjInstance>(klass);  // Replace the class object being called to its instance.
        decltype(klass->methods)::iterator initializer;
        if ((initializer = klass->methods.find(initString)) != klass->methods.end()) {
          call(initializer->second, argCount);
        } else if (argCount > 0) {
          throwRuntimeError("expected 0 arguments but got " + std::to_string(argCount) + ".");
        }
        return;
      }
      case ObjType::OBJ_BOUND_METHOD: {
       /**
                    frame->slots                          StackTop
                          │                                   │
                          ▼        A() CallFrame              ▼
            0──────1──────2─────────────────────3──────4──────5
            │script│  10  │    BoundMethod A    │ arg1 │ arg2 │
            └──────┴──────0─────────────────────1──────2──────3                                
                         
                                  ┌────────┐
                                  │ method │───────────┐
                                  └────────┘    ┼      ▼      ┼
            0──────1──────2─────────────────────3──────4──────5
            │script│  10  │   instance (this)   │ arg1 │ arg2 │
            └──────┴──────0─────────────────────1──────2──────3
                          ┼        Slot 0       ┼ 
        */
        const auto bound = calleeObj->cast<ObjBoundMethod>();
        *(stackTop - argCount - 1) = bound->receiver;  // Save the instance to slot 0 of the call frame.
        call(bound->method, argCount);
        return;
      }
      case ObjType::OBJ_CLOSURE:
      case ObjType::OBJ_FUNCTION: {
        call(calleeObj, argCount);  // OBJ_FUNCTION, OBJ_CLOSURE.
        return;
      }
      default: ;
    }
  }
  throwRuntimeError("can only call functions and classes.");
}

void VM::defineMethod(Obj* name) {
  const auto method = peekOfType<Obj*>();
  auto klass = peekOfType<Obj*>(1)->cast<ObjClass>();
  klass->methods[name] = method;
  pop();  // Pop the method function (or closure).
}

/**
 * Bind calling method to its class instance.
*/
void VM::bindMethod(ObjClass* klass, Obj* name) {
  const auto& method =klass->methods.find(name);
  if (method == klass->methods.end()) {
    throwRuntimeError("undefined property '" + name->cast<ObjString>()->str + "'.");
  }
  auto bound = mem->makeObj<ObjBoundMethod>(peek(0), method->second);
  pop();
  push(bound);  // Save the decorated method onto the stack.
}

void VM::invokeFromClass(ObjClass* klass, Obj* name, uint8_t argCount) {
  const auto& method =klass->methods.find(name); 
  if (method == klass->methods.end()) {
    throwRuntimeError("undefined property '" + name->cast<ObjString>()->str + "'.");
  }
  call(method->second, argCount);
}

void VM::invoke(Obj* name, uint8_t argCount) {
  /**
                ┌───────┐ name ┌────────┐
    (Slot 0) -> | class |─────>│ method │────────────┐
                └───────┘      └────────┘            |
                                              ┼      ▼      ┼
          0──────1──────2─────────────────────3──────4──────5
          │script│  10  │   instance (this)   │ arg1 │ arg2 │
          └──────┴──────0─────────────────────1──────2──────3
                        ┼        Slot 0       ┼ 
  */
  Obj* receiverObj = nullptr;
  if (!std::holds_alternative<Obj*>(peek(argCount)) 
    || (receiverObj = std::get<Obj*>(peek(argCount)))->type != ObjType::OBJ_INSTANCE) {
    throwRuntimeError("only instances have methods.");
  }
  auto instance = receiverObj->cast<ObjInstance>();
  // Field access gose first.
  const auto valIt = instance->fields.find(name);
  if (valIt != instance->fields.end()) {
    *(stackTop - argCount - 1) = valIt->second;
    return callValue(valIt->second, argCount);
  } else {
    return invokeFromClass(instance->klass, name, argCount);
  }
}

VMResult VM::run(void) {
  #define NUM_BINARY_OP(op) \
    do { \
      checkNumberOperands(2); \
      auto b = std::get<typeRuntimeNumericValue>(pop());  /* The left operand would be at the bottom. */ \
      auto a = std::get<typeRuntimeNumericValue>(pop()); \
      push(a op b); \
    } while (false)
  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ■ ");
    for (auto it = stack.cbegin(); it < stackTop; it++) {
      printf("[ ");
      printValue(*it);
      printf(" ] ");
    }
    printf("<-\n");
    auto currentIp = currentFrame->ip;
    ChunkDebugger::disassembleInstruction(retrieveObjFunc(currentFrame->frameEntity)->chunk, currentIp);
#endif
    const auto instruction = readByte();
    switch (instruction) {
      case OpCode::OP_ADD: {
        const auto y = pop();
        const auto x = pop();
        if (isNumericValue(x) && isNumericValue(y)) {
          push(std::get<typeRuntimeNumericValue>(x) + std::get<typeRuntimeNumericValue>(y));
          break;
        } else if ((isObjStringValue(x) || isObjStringValue(y))) {
          const auto str = stringifyVariantValue(x) + stringifyVariantValue(y);
          push(internedConstants.add(str));
          break;
        }  
        throwRuntimeError("invalid operand types for \"+\" operator.");
      }
      case OpCode::OP_SUBTRACT: NUM_BINARY_OP(-); break;
      case OpCode::OP_MULTIPLY: NUM_BINARY_OP(*); break;
      case OpCode::OP_DIVIDE: NUM_BINARY_OP(/); break;
      case OpCode::OP_NEGATE: {
        checkNumberOperands(1);
        *top() = -std::get<typeRuntimeNumericValue>(*top());
        break;
      }
      case OpCode::OP_RETURN: {
        const auto result = pop();
        closeUpvalues(currentFrame->slots);
        frameCount--;
        if (frameCount == 0) {
          pop();  // Dicard the main script function.
          return VMResult::INTERPRET_OK;
        }
        stackTop = currentFrame->slots;  // Discard all the unused locals.
        push(result);
        currentFrame = &frames[frameCount - 1];
        break;
      }
      case OpCode::OP_CONSTANT: {
        push(readConstant());
        break;
      }
      case OpCode::OP_NIL: push(std::monostate {}); break;
      case OpCode::OP_TRUE: push(true); break;
      case OpCode::OP_FALSE: push(false); break;
      case OpCode::OP_NOT: push(isFalsey(pop())); break;
      case OpCode::OP_EQUAL: {
        const auto x = pop();
        const auto y = pop();
        push(x == y);
        break;
      }
      case OpCode::OP_GREATER: NUM_BINARY_OP(>); break;
      case OpCode::OP_LESS: NUM_BINARY_OP(<); break;
      case OpCode::OP_POP: pop(); break;
      case OpCode::OP_DEFINE_GLOBAL: {
        globals[readConstantOfType<Obj*>()] = pop();
        break;
      }
      case OpCode::OP_GET_GLOBAL: {
        const auto name = readConstantOfType<Obj*>();
        const auto value = globals.find(name);
        if (value == globals.end()) {
          throwRuntimeError("undefined variable '" + name->cast<ObjString>()->str + "'.");
        }
        push(value->second);
        break;
      }
      case OpCode::OP_SET_GLOBAL: {
        const auto name = readConstantOfType<Obj*>();
         if (!globals.contains(name)) {
          throwRuntimeError("undefined variable '" + name->cast<ObjString>()->str + "'.");
         }
        globals[name] = peek(0);  // Assignment expression doesn’t pop the value off the stack.
        break;
      }
      case OpCode::OP_GET_LOCAL: {
        push(*(currentFrame->slots + readByte()));  // Take the operand from stack (local slot), and load the value.
        break;
      }
      case OpCode::OP_SET_LOCAL: {
        *(currentFrame->slots + readByte()) = peek(0);
        break;
      }
      case OpCode::OP_JUMP_IF_FALSE: {
        auto offset = readShort();
        if (isFalsey(peek(0))) currentFrame->ip += offset;
        break;
      }
      case OpCode::OP_LOOP: {
        currentFrame->ip -= readShort();
        break;
      }
      case OpCode::OP_JUMP: {
        currentFrame->ip += readShort();
        break;
      }
      case OpCode::OP_CALL: {
        /**
                    frame->slots           StackTop
                          │                    │
                          ▼                    ▼
            0──────1──────2──────3──────4──────5
            │script│  10  │ fn A │ arg1 │ arg2 │
            └──────┴──────0──────1──────2──────3
                          ┼   A() CallFrame    ┼
        */
        const auto argCount = readByte();
        callValue(peek(argCount), argCount);
        break;
      }
      case OpCode::OP_GET_UPVALUE: {
        const auto slot = readByte();
        push(*currentFrame->frameEntity->cast<ObjClosure>()->upvalues[slot]->location);
        break;
      }
      case OpCode::OP_SET_UPVALUE: {
        const auto slot = readByte();
        *currentFrame->frameEntity->cast<ObjClosure>()->upvalues[slot]->location = peek(0);
        break;
      }
      case OpCode::OP_CLOSURE: {
        auto closure = mem->makeObj<ObjClosure>(retrieveObjFunc(readConstantOfType<Obj*>()));
        push(closure);
        for (uint32_t i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = readByte();
          uint8_t index = readByte();
          if (isLocal == 1) {
            closure->upvalues[i] = captureUpvalue(&*(currentFrame->slots + index));
          } else {
            closure->upvalues[i] = currentFrame->frameEntity->cast<ObjClosure>()->upvalues[index];
          }
        }
        break;
      }
      case OpCode::OP_CLOSE_UPVALUE: {
        closeUpvalues(&*(stackTop - 1));  // Hoisting the local to heap.
        pop();  // Then, discard it from the stack.
        break;
      }
      case OpCode::OP_CLASS: {
        const auto name = readConstantOfType<Obj*>();
        push(mem->makeObj<ObjClass>(name->cast<ObjString>()));
        break;
      }
      case OpCode::OP_GET_PROPERTY: {
        Obj* obj = nullptr;
        if (!std::holds_alternative<Obj*>(peek()) || (obj = std::get<Obj*>(peek()))->type != ObjType::OBJ_INSTANCE) {
          throwRuntimeError("only instances have properties.");
        }
        auto instance = obj->cast<ObjInstance>();
        const auto name = readConstantOfType<Obj*>();
        const auto valIt = instance->fields.find(name);
        if (valIt != instance->fields.end()) {
          pop();  // Pop the instance object.
          push(valIt->second);
        } else {
          bindMethod(instance->klass, name);
        }
        break;
      }
      case OpCode::OP_SET_PROPERTY: {
        Obj* obj = nullptr;
        if (!std::holds_alternative<Obj*>(peek(1)) || (obj = std::get<Obj*>(peek(1)))->type != ObjType::OBJ_INSTANCE) {
          throwRuntimeError("only instances have properties.");
        }
        auto instance = obj->cast<ObjInstance>();
        const auto name = readConstantOfType<Obj*>();
        instance->fields[name] = peek(0);
        const auto value = pop();
        pop();
        push(value);  // Leave the assigned value on the stack.
        break;
      }
      case OpCode::OP_METHOD: {
        defineMethod(readConstantOfType<Obj*>());
        break;
      }
      case OpCode::OP_INVOKE: {
        const auto methodName = readConstantOfType<Obj*>();
        const auto argCount = readByte();
        invoke(methodName, argCount);
        currentFrame = &frames[frameCount - 1];  // Update frame to the latest called method.
        break;
      }
      case OpCode::OP_INHERIT: {
        Obj* superclass = nullptr;
        if (!std::holds_alternative<Obj*>(peek(1)) || (superclass = std::get<Obj*>(peek(1)))->type != ObjType::OBJ_CLASS) {
          throwRuntimeError("superclass must be a class.");
        }
        auto subclass = peekOfType<Obj*>(0);
        for (const auto& entity : superclass->cast<ObjClass>()->methods) {  // Copy the inherited methods to subclass.
          subclass->cast<ObjClass>()->methods[entity.first] = entity.second;
        }
        break;
      }
      case OpCode::OP_GET_SUPER: {
        const auto methodName = readConstantOfType<Obj*>();
        const auto superclass = std::get<Obj*>(pop())->cast<ObjClass>();
        bindMethod(superclass, methodName);  // The instance is on the top of stack.
        break;
      }
      case OpCode::OP_SUPER_INVOKE: {
        const auto methodName = readConstantOfType<Obj*>();
        const auto argCount = readByte();
        const auto superclass = std::get<Obj*>(pop())->cast<ObjClass>();
        invokeFromClass(superclass, methodName, argCount);
        currentFrame = &frames[frameCount - 1];  // Update frame to the latest called method.
        break;
      }
    }
  }
  #undef NUM_BINARY_OP
}

void VM::stackTrace(void) {
  for (auto i = frameCount; i >= 1; i--) {
    auto frame = &frames[i - 1];
    auto function = retrieveObjFunc(frame->frameEntity);
    auto instruction = frame->ip - function->chunk.code.cbegin() - 1;
    fprintf(stderr, "[Line %zu] in ", function->chunk.getLine(instruction));
    if (function->name == NULL) {
      fprintf(stderr, "script.\n");
    } else {
      fprintf(stderr, "%s().\n", function->name->str.c_str());
    }
  }
}

VMResult VM::interpret(void) {
  if (!isStatusOk) return VMResult::INTERPRET_RUNTIME_ERROR;
  try {
    const auto result = run();
    freeVM();
    return result;
  } catch(const VMError& err) {
    Error::vmError(err);
    stackTrace();
    return VMResult::INTERPRET_RUNTIME_ERROR;
  }
}
