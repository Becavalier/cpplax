#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "lib/expr.h"
#include "lib/helper.h"
#include "lib/error.h"
#include "lib/stmt.h"
#include "lib/helper.h"
#include "lib/env.h"

struct Interpreter;
// This is an "invokable" class for function and method.
struct Invokable {  
  virtual size_t arity() = 0;
  virtual typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) = 0;
  virtual ~Invokable() {}
};

struct Interpreter : public ExprVisitor, public StmtVisitor, public std::enable_shared_from_this<Interpreter> {
  const std::shared_ptr<Env> globals = std::make_shared<Env>();
  std::shared_ptr<Env> env = globals;  // Global scope.
  struct BlockExecution {
    Interpreter* thisPtr;
    const std::vector<std::shared_ptr<Stmt>>& statements;
    const std::shared_ptr<Env> previousEnv;
    const std::shared_ptr<Env> env;
    void execute(void) {
      thisPtr->env = env;
      for (auto& statement : statements) {
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
  typeRuntimeValue evaluate(std::shared_ptr<Expr> expr) {
    return expr->accept(shared_from_this());
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
      default:;
    }
    return std::monostate {};
  }
  typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr> expr) override {
    auto value = evaluate(expr->value);
    env->assign(expr->name, value);
    return value;
  }
  typeRuntimeValue visitVariableExpr(std::shared_ptr<const VariableExpr> expr) override { return env->get(expr->name); }
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
      throw RuntimeError { expr->paren, "expected " + std::to_string(function->arity()) + " arguments but got " + std::to_string( arguments.size()) + "." };
    }
    return function->invoke(this, arguments);
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
    } else if (stmt->thenBranch != nullptr) {
      execute(stmt->elseBranch);
    }
  }
  void visitWhileStmt(std::shared_ptr<const WhileStmt> stmt) override {
    while (isTruthy(evaluate(stmt->condition))) {
      execute(stmt->body);
    }
  }
  void visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) override;
  void executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements, std::shared_ptr<Env> env) {
    BlockExecution { this, statements, this->env, env, }.execute();
  }
  void execute(std::shared_ptr<Stmt> stmt) {
    stmt->accept(shared_from_this());
  }
  void interpret(std::vector<std::shared_ptr<Stmt>> statements) {
    try {
      for (auto& statement : statements) {
        execute(statement);
      }
    } catch (RuntimeError error) {
      Error::runtimeError(error);
    }
  }
};

struct Function : public Invokable {
  Function(std::shared_ptr<const FunctionStmt> declaration) : declaration(declaration) {}
  size_t arity() override {
    return declaration->parames.size();
  };
  typeRuntimeValue invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) override {
    // Each function gets its own environment where it stores those variables.
    auto env = std::make_shared<Env>(interpreter->globals);
    for (auto i = 0; i < declaration->parames.size(); i++) {
      env->define(declaration->parames.at(i).get().lexeme, arguments.at(i));  // Save passing arguments into the env.
    }
    interpreter->executeBlock(declaration->body, env);
    return std::monostate {};
  };
 private:
  const std::shared_ptr<const FunctionStmt> declaration;
};


#endif
