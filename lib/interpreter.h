#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <variant>
#include "./expr.h"
#include "./helper.h"
#include "./error.h"
#include "./stmt.h"
#include "./helper.h"
#include "./env.h"

// Forward declaration.
struct Interpreter;

/**
 * Class "Invokable", for function and method.
*/
struct Invokable {  
  virtual size_t arity() = 0;
  virtual typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) = 0;
  virtual ~Invokable() {}
};

/**
 * Class "Function".
*/
struct Function : public Invokable {
  Function(std::shared_ptr<const FunctionStmt> declaration, std::shared_ptr<Env> closure) : declaration(declaration), closure(closure) {}
  size_t arity() override {
    return declaration->parames.size();
  }
  typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override;
 private:
  const std::shared_ptr<const FunctionStmt> declaration;
  std::shared_ptr<Env> closure;  // Capture the env at the definition place.
};

/**
 * Class "Class".
*/
struct Class : public Invokable, public std::enable_shared_from_this<Class> {
  const std::string& name;
  explicit Class(const std::string& name, std::unordered_map<std::string, std::shared_ptr<Function>>& methods) : name(name), methods(methods) {}
  size_t arity() override {
    return 0;
  }
  typeRuntimeValue invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) override {
    return std::make_shared<ClassInstance>(shared_from_this());  // Return a new instance.
  }
  std::shared_ptr<Function> findMethod(const std::string& name) {
    if (methods.contains(name)) return methods[name];
    return nullptr;
  }
 private:
  std::unordered_map<std::string, std::shared_ptr<Function>> methods;
};


/**
 * Class "ClassInstance".
*/
struct ClassInstance {
  explicit ClassInstance(std::shared_ptr<Class> thisClass) : thisClass(thisClass) {}
  typeRuntimeValue get(const Token& name);
  void set(const Token& name, typeRuntimeValue value) {
    fields[name.lexeme] = value;
  }
 private:
  std::shared_ptr<Class> thisClass;
  std::unordered_map<std::string, typeRuntimeValue> fields;
};

struct ReturnException : public std::exception {
  const typeRuntimeValue value;
  explicit ReturnException(const typeRuntimeValue& value) : value(value) {}
};


/**
 * Class "Interpreter".
*/
struct Interpreter : public ExprVisitor, public StmtVisitor {
  const std::shared_ptr<Env> globals = std::make_shared<Env>();
  // Saving the scope steps for locals.
  std::unordered_map<Expr::sharedConstExprPtr, int> locals;  
  std::shared_ptr<Env> env = globals;  // Global scope.
  struct BlockExecution {
    Interpreter* thisPtr;
    const std::vector<Stmt::sharedStmtPtr>& statements;
    const std::shared_ptr<Env> previousEnv;
    const std::shared_ptr<Env> env;
    void execute(void) {
      thisPtr->env = env; 
      for (const auto& statement : statements) {
        thisPtr->execute(statement);
      }
    }
    ~BlockExecution() {
      thisPtr->env = previousEnv;
    }
  };
  Interpreter() {
    // Define and add internal functions.
    class : public Invokable {
      size_t arity() override { return 0; };
      typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override {
        return static_cast<double>(
          std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
          ).count()
        );
      };
    } clockDef;
    globals->define("clock", std::make_shared<decltype(clockDef)>(clockDef));
  }
  typeRuntimeValue evaluate(Expr::sharedExprPtr expr) {
    return expr->accept(this);
  }
  void resolve(Expr::sharedConstExprPtr expr, int depth) {
    locals[expr] = depth;
  }
  auto isTruthy(typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return false;
    if (std::holds_alternative<bool>(obj)) return std::get<bool>(obj);
    if (std::holds_alternative<std::string>(obj)) return std::get<std::string>(obj).size() > 0;
    if (std::holds_alternative<double>(obj)) return !isDoubleEqual(std::get<double>(obj), 0);
    return true;
  }
  void checkNumberOperand(const Token& op, const typeRuntimeValue& operand) const {
    if (std::holds_alternative<double>(operand)) return;
    throw RuntimeError { op, "operand must be a number." };
  }
  void checkNumberOperands(const Token& op, const typeRuntimeValue& x, const typeRuntimeValue& y) const {
    if (std::holds_alternative<double>(x) && std::holds_alternative<double>(y)) return;
    throw RuntimeError { op, "operands must be numbers." };
  }
  typeRuntimeValue lookUpVariable(const Token& name, Expr::sharedConstExprPtr expr) {
    const auto distance = locals.find(expr);
    if (distance != locals.end()) {
      return env->getAt(distance->second, name);
    } else {
      return globals->get(name);
    }
  }
  typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr> expr) override { 
    return expr->value;  // Return as runtime values.
  }  
  typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr> expr) override { 
    return evaluate(expr->expression);  // Recursively evaluate that subexpression and return it.
  }  
  typeRuntimeValue visitUnaryExpr(std::shared_ptr<const UnaryExpr> expr) override {
    auto right = evaluate(expr->right);
    switch (expr->op.type) {
      case TokenType::MINUS: {
        checkNumberOperand(expr->op, right);
        return -std::get<double>(right);
      }
      case TokenType::BANG: {
        return !isTruthy(right);
      }
      default: ;
    }
    return std::monostate {};
  }
  typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr> expr) override {
    auto value = evaluate(expr->value);
    auto distance = locals.find(expr);
    if (distance != locals.end()) {
      env->assignAt(distance->second, expr->name, value);
    } else {
      globals->assign(expr->name, value);
    }
    return value;
  }
  typeRuntimeValue visitVariableExpr(std::shared_ptr<const VariableExpr> expr) override { 
    return lookUpVariable(expr->name, expr);
  }
  typeRuntimeValue visitLogicalExpr(std::shared_ptr<const LogicalExpr> expr) override {
    auto left = evaluate(expr->left);
    if (expr->op.type == TokenType::OR) {
      if (isTruthy(left)) return left;
    } else {
      if (!isTruthy(left)) return left;
    }
    return evaluate(expr->right);
  }
  typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr> expr) override {
    // Evaluate the operands in left-to-right order.
    auto left = evaluate(expr->left);
    auto right = evaluate(expr->right);
    switch (expr->op.type) {
      case TokenType::BANG_EQUAL: {
        return left != right;
      }
      case TokenType::EQUAL_EQUAL: {
        return left == right;
      }
      case TokenType::GREATER: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) > std::get<double>(right);
      }
      case TokenType::GREATER_EQUAL: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) >= std::get<double>(right);
      }
      case TokenType::LESS: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) < std::get<double>(right);
      }
      case TokenType::LESS_EQUAL: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) <= std::get<double>(right);
      }
      case TokenType::MINUS: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) - std::get<double>(right);
      }
      case TokenType::PLUS: {
        if (std::holds_alternative<double>(left)) {
          if (std::holds_alternative<double>(right)) {
            return std::get<double>(left) + std::get<double>(right);
          }
          if (std::holds_alternative<std::string>(right)) {
            return (std::ostringstream {} << std::get<double>(left) << std::get<std::string>(right)).str();
          }
        }
        if (std::holds_alternative<std::string>(left)) {
          if (std::holds_alternative<double>(right)) {
            return (std::ostringstream {} << std::get<std::string>(left) << std::get<double>(right)).str();
          }
          if (std::holds_alternative<std::string>(right)) {
            return (std::ostringstream {} << std::get<std::string>(left) << std::get<std::string>(right)).str();
          }
        }
        throw RuntimeError { expr->op, "operand must be type of number or string." };
      }
      case TokenType::SLASH: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) / std::get<double>(right);
      }
      case TokenType::STAR: {
        checkNumberOperands(expr->op, left, right);
        return std::get<double>(left) * std::get<double>(right);
      }
      default:;
    }
    return std::monostate {};
  }
  typeRuntimeValue visitCallExpr(std::shared_ptr<const CallExpr> expr) override {
    auto callee = evaluate(expr->callee);  // "primary" -> TokenType::IDENTIFIER -> VariableExpr -> Env.get.
    std::vector<typeRuntimeValue> arguments;
    for (const auto& argument : expr->arguments) {
      arguments.push_back(evaluate(argument));
    }
    // Check if it's callable.
    if (!std::holds_alternative<std::shared_ptr<Invokable>>(callee)) {
      throw RuntimeError { expr->paren, "can only call functions and classes." };
    }
    const auto function = std::static_pointer_cast<Invokable>(std::get<std::shared_ptr<Invokable>>(callee));
    // Check if it matches the calling arity.
    if (arguments.size() != function->arity()) {
      throw RuntimeError { 
        expr->paren, 
        "expected " + std::to_string(function->arity()) + " arguments but got " + std::to_string( arguments.size()) + "." 
      };
    }
    return function->invoke(this, arguments);
  }
  typeRuntimeValue visitGetExpr(std::shared_ptr<const GetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a getter.
    if (vp != nullptr) return (*vp)->get(expr->name);
    throw RuntimeError { expr->name, "only instances have properties." };
  }
  typeRuntimeValue visitSetExpr(std::shared_ptr<const SetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a setter.
    if (vp == nullptr) throw RuntimeError { expr->name, "only instances have fields." };
    auto value = evaluate(expr->value);
    (*vp)->set(expr->name, value);
    return value;
  }
  void visitExpressionStmt(std::shared_ptr<const ExpressionStmt> stmt) override {
    evaluate(stmt->expression);
  }
  void visitPrintStmt(std::shared_ptr<const PrintStmt> stmt) override {
    std::cout << toRawString(stringifyVariantValue(evaluate(stmt->expression)));
  }
  void visitVarStmt(std::shared_ptr<const VarStmt> stmt) override {
    typeRuntimeValue value;
    if (stmt->initializer != nullptr) {
      value = evaluate(stmt->initializer);
    }
    env->define(stmt->name.lexeme, value);
  }
  void visitBlockStmt(std::shared_ptr<const BlockStmt> stmt) override {
    executeBlock(stmt->statements, std::make_shared<Env>(env));
  }
  void visitIfStmt(std::shared_ptr<const IfStmt> stmt) override {
    if (isTruthy(evaluate(stmt->condition))) {
      execute(stmt->thenBranch);
    } else if (stmt->elseBranch != nullptr) {
      execute(stmt->elseBranch);
    }
  }
  void visitWhileStmt(std::shared_ptr<const WhileStmt> stmt) override {
    while (isTruthy(evaluate(stmt->condition))) {
      execute(stmt->body);
    }
  }
  void visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) override;
  void visitReturnStmt(std::shared_ptr<const ReturnStmt> stmt) override {
    // Unwind the call stack by exception.
    typeRuntimeValue value = std::monostate {};
    if (stmt->value != nullptr) value = evaluate(stmt->value);
    throw ReturnException { value };
  }
  void visitClassStmt(std::shared_ptr<const ClassStmt> stmt) override;
  void executeBlock(const std::vector<Stmt::sharedStmtPtr>& statements, std::shared_ptr<Env> env) {
    BlockExecution { this, statements, this->env, env, }.execute();
  }
  void execute(Stmt::sharedStmtPtr stmt) {
    stmt->accept(this);
  }
  void interpret(const std::vector<Stmt::sharedStmtPtr>& statements) {
    try {
      for (const auto& statement : statements) {
        execute(statement);
      }
    } catch (const RuntimeError& runtimeError) {
      Error::runtimeError(runtimeError);
    }
  }
};

// Post definitions.
void Interpreter::visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) {
  std::shared_ptr<Invokable> invoker = std::make_shared<Function>(stmt, env);  // Up-casting.
  env->define(stmt->name.lexeme, invoker);
}

void Interpreter::visitClassStmt(std::shared_ptr<const ClassStmt> stmt) {
  // Turn the class syntax node into its runtime representation (class Class).
  env->define(stmt->name.lexeme, std::monostate {});
  std::unordered_map<std::string, std::shared_ptr<Function>> methods;
  for (const auto& method : stmt->methods) {
    auto function = std::make_shared<Function>(method, env);
    methods[method->name.lexeme] = function;
  }
  auto thisClass = std::make_shared<Class>(stmt->name.lexeme, methods);
  env->assign(stmt->name, thisClass);
}

typeRuntimeValue Function::invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) {
  // Each function gets its own environment where it stores those variables.
  // "env -> closure -> global".
  auto env = std::make_shared<Env>(closure);
  for (size_t i = 0; i < declaration->parames.size(); i++) {
    // Save the passing arguments into the env.
    env->define(declaration->parames.at(i).get().lexeme, arguments.at(i));  
  }
  try {
    interpreter->executeBlock(declaration->body, env);
  } catch (const ReturnException& ret) {
    return ret.value;
  }
  return std::monostate {};
}

typeRuntimeValue ClassInstance::get(const Token& name) {
  if (fields.contains(name.lexeme)) return fields[name.lexeme];
  auto method = thisClass->findMethod(name.lexeme);
  if (method != nullptr) return method;
  throw RuntimeError { name, "undefined property '" + name.lexeme + "'." };
}

#endif
