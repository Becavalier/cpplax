#ifndef	_ENV_H
#define	_ENV_H

#include <unordered_map>
#include <string>
#include <memory>
#include "./type.h"
#include "./token.h"
#include "./error.h"

class Env : public std::enable_shared_from_this<Env> {
  std::shared_ptr<Env> enclosing;
  std::unordered_map<std::string, typeRuntimeValue> values;
 public: 
  Env(void) : enclosing(nullptr) {};  // For the global scope’s environment.
  explicit Env(std::shared_ptr<Env> enclosing) : enclosing(enclosing) {};
  void define(const std::string& name, const typeRuntimeValue& value) {
    values[name] = value;
  } 
  std::shared_ptr<Env> ancestor(int distance) {
    auto env = shared_from_this();
    for (int i = 0; i < distance; i++) {
      env = env->enclosing;
    }
    return env;
  }
  typeRuntimeValue getAt(int distance, const std::string& name) {
    return ancestor(distance)->values[name];
  }
  typeRuntimeValue get(const Token& name) {
    const auto target = values.find(name.lexeme);
    if (target != values.end()) return target->second;
    if (enclosing != nullptr) return enclosing->get(name);  // Look up from the upper scope.
    throw RuntimeError(name, ("undefined variable '" + name.lexeme + "'.").data());
  }
  void assignAt(int distance, const Token& name, const typeRuntimeValue& value) {
    ancestor(distance)->values[name.lexeme] = value;
  }
  void assign(const Token& name, const typeRuntimeValue& value) {
    if (values.contains(name.lexeme)) {
      values[name.lexeme] = value;
      return;
    }
    if (enclosing != nullptr) {
      enclosing->assign(name, value);
      return;
    }
    throw RuntimeError(name, ("undefined variable '" + name.lexeme + "'.").data());
  }
};

#endif
