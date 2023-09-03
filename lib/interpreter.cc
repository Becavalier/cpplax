#include "lib/interpreter.h"

// Below definition depends on the definition of "Function" class.
void Interpreter::visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) {
  std::shared_ptr<Invokable> invoker = std::make_shared<Function>(stmt);  // Up-casting.
  env->define(stmt->name.lexeme, invoker);
}
