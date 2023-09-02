#include <unordered_map>
#include <string>
#include <memory>
#include "lib/type.h"
#include "lib/token.h"
#include "lib/error.h"

class Env {
  std::shared_ptr<Env> enclosing;
  std::unordered_map<std::string, typeRuntimeValue> values;
 public:
  Env(void) : enclosing(nullptr) {};  // For the global scope’s environment.
  Env(std::shared_ptr<Env> enclosing) : enclosing(enclosing) {};
  void define(const std::string& name, const typeRuntimeValue& value) {
    values[name] = value;
  }
  typeRuntimeValue get(const Token& name) {
    const auto target = values.find(name.lexeme);
    if (target != values.end()) {
      return target->second;
    }
    if (enclosing != nullptr) return enclosing->get(name);  // Look up from the upper scope.
    throw RuntimeError(name, ("undefined variable '" + name.lexeme + "'.").data());
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
