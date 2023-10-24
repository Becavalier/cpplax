#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

/**
 * This is the implementation of a AST tree-walking interpreter.
 * 
 * 1. Overhead is high due to the dynamic dispatch of each "visit method".
 * 2. Sprinkling data across the heap in a loosely connected web of objects does bad things for spatial locality.
 * 3. Not memory-efficient because of the way of AST nodes' representation in memory.
*/

#include <memory>
#include <vector>
#include <string>
#include <ctime>
#include <unordered_map>
#include <variant>
#include <optional>
#include "./expr.h"
#include "./helper.h"
#include "./error.h"
#include "./stmt.h"
#include "./helper.h"
#include "./env.h"
#include "./type.h"
#include "./common.h"

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
  std::unordered_map<Expr::sharedConstExprPtr, size_t> locals;  
  std::shared_ptr<Env> env = globals;  // Global scope.
  struct BlockExecution {
    Interpreter* thisPtr;
    const std::vector<Stmt::sharedStmtPtr>& statements;
    const std::shared_ptr<Env> previousEnv;
    const std::shared_ptr<Env> env;
    void execute(void) {
      thisPtr->env = env; // Set up new env for the new scope.
      for (const auto& statement : statements) {
        thisPtr->execute(statement);
      }
    }
    ~BlockExecution() {
      thisPtr->env = previousEnv;  // Restore the reference to the previous env.
    }
  };
  Interpreter() {
    // Define internal functions.
    class InternalFunClockDef : public Invokable {
      std::string toString(void) override { 
        return "<fn native>"; 
      }
      size_t arity() override { return 0; };
      typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override {
        return static_cast<double>(clock()) / CLOCKS_PER_SEC;
      };
    };
    class InternalFunPrintDef : public Invokable {
      std::string toString(void) override { 
        return "<fn native>"; 
      }
      size_t arity() override { return 1; };
      typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>& arguments) override {
        std::cout << unescapeStr(stringifyVariantValue(arguments.back()));
        return std::monostate {};
      };
    };
    globals->define("clock", std::make_shared<InternalFunClockDef>());
    globals->define("print", std::make_shared<InternalFunPrintDef>());
  }
  typeRuntimeValue evaluate(Expr::sharedExprPtr expr) {
    return expr->accept(this);
  }
  void resolve(Expr::sharedConstExprPtr expr, size_t depth) {
    locals[expr] = depth;  // Save the "Expr" as the key in case of any conflicts.
  }
  auto isTruthy(typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return false;
    if (std::holds_alternative<bool>(obj)) return std::get<bool>(obj);
    return true;
  }
  void checkNumberOperand(const Token& op, const typeRuntimeValue& operand) const {
    if (std::holds_alternative<typeRuntimeNumericValue>(operand)) return;
    throw TokenError { op, "operand must be a number." };
  }
  void checkNumberOperands(const Token& op, const typeRuntimeValue& left, const typeRuntimeValue& right) const {
    if (std::holds_alternative<typeRuntimeNumericValue>(left) && std::holds_alternative<typeRuntimeNumericValue>(right)) return;
    throw TokenError { op, "operands must be numbers." };
  }
  typeRuntimeValue lookUpVariable(const Token& name, Expr::sharedConstExprPtr expr) {
    const auto distance = locals.find(expr);
    if (distance != locals.end()) {
      return env->getAt(distance->second, name.lexeme);
    } else {
      return globals->get(name);  // Throw if variable is undefined.
    }
  }
  typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr> expr) override { 
    return expr->value;  // Return as runtime values.
  }  
  typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr> expr) override { 
    return evaluate(expr->expression);  // Recursively evaluate its subexpression and return the result.
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
    if (expr->op.type == TokenType::OR) {  // Short-circuit.
      if (isTruthy(left)) return left;
    } else {
      if (!isTruthy(left)) return left;
    }
    return evaluate(expr->right);
  }
  typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr> expr) override {
    #define RET_NUM_BINARY_OP(token, op, left, right) \
      checkNumberOperands(token, left, right); \
      return std::get<typeRuntimeNumericValue>(left) op std::get<typeRuntimeNumericValue>(right);
    /**
     * Another semantic choice: 
     * We could have instead specified that the left operand is checked, -
     * before even evaluating the right.
    */
    // Evaluate the operands in "left-to-right" order.
    const auto left = evaluate(expr->left);
    const auto right = evaluate(expr->right);
    switch (expr->op.type) {
      case TokenType::BANG_EQUAL: {
        return left != right;
      }
      case TokenType::EQUAL_EQUAL: {
        return left == right;
      }
      case TokenType::GREATER: {
        RET_NUM_BINARY_OP(expr->op, >, left, right)
      }
      case TokenType::GREATER_EQUAL: {
        RET_NUM_BINARY_OP(expr->op, >=, left, right)
      }
      case TokenType::LESS: {
        RET_NUM_BINARY_OP(expr->op, <, left, right)
      }
      case TokenType::LESS_EQUAL: {
        RET_NUM_BINARY_OP(expr->op, <=, left, right)
      }
      case TokenType::MINUS: {
        RET_NUM_BINARY_OP(expr->op, -, left, right)
      } 
      case TokenType::PLUS: {
        const auto str = std::visit([](auto&& argLeft, auto&& argRight) -> std::optional<typeRuntimeValue> {
          using T = std::decay_t<decltype(argLeft)>;
          using K = std::decay_t<decltype(argRight)>;
          if constexpr (std::is_same_v<T, typeRuntimeNumericValue>) {
            if constexpr (std::is_same_v<K, typeRuntimeNumericValue>) {
              return argLeft + argRight;
            } else if constexpr (std::is_same_v<K, std::string_view>) {
              return std::to_string(argLeft) + std::string { argRight };
            } else if constexpr (std::is_same_v<K, std::string>) {
              return std::to_string(argLeft) + argRight;
            }
          } else if constexpr (std::is_same_v<T, std::string_view>) {
            const auto argLeftStr = std::string { argLeft };
            if constexpr (std::is_same_v<K, typeRuntimeNumericValue>) {
              return argLeftStr + std::to_string(argRight);
            } else if constexpr (std::is_same_v<K, std::string_view>) {
              return argLeftStr + std::string { argRight };
            } else if constexpr (std::is_same_v<K, std::string>) {
              return argLeftStr + argRight;
            }
          } else if constexpr (std::is_same_v<T, std::string>) {
            if constexpr (std::is_same_v<K, typeRuntimeNumericValue>) {
              return argLeft + std::to_string(argRight);
            } else if constexpr (std::is_same_v<K, std::string_view>) {
              return argLeft + std::string { argRight };
            } else if constexpr (std::is_same_v<K, std::string>) {
              return argLeft + argRight;
            }
          }
          return std::nullopt;
        }, left, right);
        if (!str.has_value()) throw TokenError { expr->op, "operand must be type of number or string." };
        return str.value();
      }
      case TokenType::SLASH: {
        RET_NUM_BINARY_OP(expr->op, /, left, right)
      }
      case TokenType::STAR: {
        RET_NUM_BINARY_OP(expr->op, *, left, right)
      }
      default:;
    }
    return std::monostate {};
    #undef NUM_BINARY_OP
  }
  typeRuntimeValue visitCallExpr(std::shared_ptr<const CallExpr> expr) override {
    auto callee = evaluate(expr->callee);  // "primary" -> TokenType::IDENTIFIER -> VariableExpr -> Env.get.
    std::vector<typeRuntimeValue> arguments;
    for (const auto& argument : expr->arguments) {
      arguments.push_back(evaluate(argument));  // Save evaluated args.
    }
    if (!std::holds_alternative<std::shared_ptr<Invokable>>(callee)) {  // Check if it's callable.
      throw TokenError { expr->paren, "can only call functions and classes." };
    }
    const auto function = std::get<std::shared_ptr<Invokable>>(callee);
    if (arguments.size() != function->arity()) {  // Check if it matches the calling arity.
      throw TokenError { expr->paren, "expected " + std::to_string(function->arity()) + " arguments but got " + std::to_string( arguments.size()) + "." };
    }
    return function->invoke(this, arguments);
  }
  typeRuntimeValue visitGetExpr(std::shared_ptr<const GetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a getter.
    if (vp != nullptr) return (*vp)->get(expr->name);
    throw TokenError { expr->name, "only instances have properties." };
  }
  typeRuntimeValue visitSetExpr(std::shared_ptr<const SetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a setter.
    if (vp == nullptr) throw TokenError { expr->name, "only instances have fields." };
    auto value = evaluate(expr->value);
    (*vp)->set(expr->name, value);
    return value;
  }
  typeRuntimeValue visitSuperExpr(std::shared_ptr<const SuperExpr> expr) override {
    const auto distance = locals[expr];
    auto superClass = std::static_pointer_cast<Class>(std::get<std::shared_ptr<Invokable>>(env->getAt(distance, "super")));
    auto thisInstance = std::get<std::shared_ptr<ClassInstance>>(env->getAt(distance - 1, "this"));
    auto method = superClass->findMethod(expr->method.lexeme);
    if (method == nullptr) {
      throw TokenError { expr->method, "undefined property '" + std::string { expr->method.lexeme } + "'." };
    }
    return method->bind(thisInstance);
  }
  typeRuntimeValue visitThisExpr(std::shared_ptr<const ThisExpr> expr) override {
    return lookUpVariable(expr->keyword, expr);
  }
  void visitExpressionStmt(std::shared_ptr<const ExpressionStmt> stmt) override {
    evaluate(stmt->expression);
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
  void visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) override { 
    std::shared_ptr<Invokable> invoker = std::make_shared<Function>(stmt, env, false);  // Save closure (the definition scope) as well.
    env->define(stmt->name.lexeme, invoker); 
  };
  void visitReturnStmt(std::shared_ptr<const ReturnStmt> stmt) override {
    // Unwind the call stack by exception.
    typeRuntimeValue value = std::monostate {};
    if (stmt->value != nullptr) value = evaluate(stmt->value);
    throw ReturnException { value };
  }
  void visitClassStmt(std::shared_ptr<const ClassStmt> stmt) override {
    std::shared_ptr<Class> superClass = nullptr;
    if (stmt->superClass != nullptr) {
      const auto superClassVal = evaluate(stmt->superClass);
      const auto superClassPtr = std::get_if<std::shared_ptr<Invokable>>(&superClassVal);
      const auto invalidSuperErrorMsg = "super class must be a class.";
      if (superClassPtr == nullptr) {
        throw TokenError { stmt->superClass->name, invalidSuperErrorMsg };
      } else {
        superClass = std::dynamic_pointer_cast<Class>(*superClassPtr);
        if (superClass == nullptr) {
          throw TokenError { stmt->superClass->name, invalidSuperErrorMsg };
        }
      }
    } 
    env->define(stmt->name.lexeme, std::monostate {});
    if (superClass != nullptr) {
      env = std::make_shared<Env>(env);
      env->define("super", superClass);  // Store the reference to super class.
    }
    std::unordered_map<std::string_view, std::shared_ptr<Function>> methods;
    for (const auto& method : stmt->methods) {
      auto function = std::make_shared<Function>(method, env, method->name.lexeme == INITIALIZER_NAME);  // Capture the current env as closure, which includes the reference to super class.
      methods[method->name.lexeme] = function;
    }
    auto thisClass = std::make_shared<Class>(stmt->name.lexeme, superClass, methods);  // Store all methods (behavior) into Class.
    if (superClass != nullptr) {
      env = env->enclosing;  // Restore the env for storing class.
    }
    env->assign(stmt->name, thisClass);
  }
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
    } catch (const TokenError& tokenError) {
      Error::tokenError(tokenError);
    }
  }
};

#endif
