#include "./type.h"
#include "./stmt.h"
#include "./env.h"
#include "./interpreter.h"
#include "./token.h"

std::string Function::toString(void) { 
  return "<fn " + std::string { declaration->name.lexeme } +  ">"; 
}
size_t Function::arity() { 
  return declaration->parames.size();
}
typeRuntimeValue Function::invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) {
  /**
   * env (created when invoked) -> 
   * closure (created when visiting function stmt, saved as an Invokable) -> 
   * global.
  */
  auto env = std::make_shared<Env>(closure);  // Each function gets its own environment where it stores those variables.
  for (size_t i = 0; i < declaration->parames.size(); i++) {
    env->define(declaration->parames.at(i).get().lexeme, arguments.at(i));  // Save the passing arguments into the env.
  }
  try {
    interpreter->executeBlock(declaration->body, env);  // Replace the Interpreter's env with the given one and then execute the code.
  } catch (const ReturnException& ret) {
    if (isInitializer) return closure->getAt(0, "this");
    return ret.value;
  }
  if (isInitializer) return closure->getAt(0, "this");
  return std::monostate {};
}
std::shared_ptr<Function> Function::bind(std::shared_ptr<ClassInstance> instance) {
  auto env = std::make_shared<Env>(closure);  // Create a new environment nestled inside the method’s original closure.
  env->define("this", instance);
  return std::make_shared<Function>(declaration, env, isInitializer);
}
std::string Class::toString(void) { 
  return "<class " + std::string { name } + ">"; 
}
size_t Class::arity() {
  return initializer == nullptr ? 0 : initializer->arity();
}
typeRuntimeValue Class::invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) {
  auto instance = std::make_shared<ClassInstance>(shared_from_this());  // Return a new instance.
  if (initializer != nullptr) {
    initializer->bind(instance)->invoke(interpreter, arguments);  // Invoke the constructor.
  }
  return instance;
}
std::shared_ptr<Function> Class::findMethod(const std::string_view name) {
  if (methods.contains(name)) {
    return methods[name];
  }
  if (superClass != nullptr) {
    return superClass->findMethod(name);  // Look up from the inherited class.
  }
  return nullptr;
}
std::string ClassInstance::toString(void) { 
  return "<instance " + std::string { thisClass->name } + ">"; 
}
typeRuntimeValue ClassInstance::get(const Token& name) {  
  if (fields.contains(name.lexeme)) {
    return fields[name.lexeme];  // Access fields in an instance.
  }
  const auto& method = thisClass->findMethod(name.lexeme);  // Otherwise, access class method (fields shadow methods).
  if (method != nullptr) {
    return method->bind(shared_from_this());  // Change method's closure and bind it to new scope.
  }
  throw TokenError { name, "undefined property '" + std::string { name.lexeme } + "'." };
}
void ClassInstance::set(const Token& name, typeRuntimeValue value) {
  fields[name.lexeme] = value;
}
