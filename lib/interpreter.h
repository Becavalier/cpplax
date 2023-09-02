#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

#include <memory>
#include <vector>
#include "lib/expr.h"
#include "lib/helper.h"
#include "lib/error.h"
#include "lib/stmt.h"
#include "lib/helper.h"
#include "lib/env.h"

// The evaluation result should be Token::typeLiteral.
class Interpreter : public ExprVisitor, public StmtVisitor {
  std::shared_ptr<Env> env = std::make_shared<Env>();  // Global scope.
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
  typeRuntimeValue evaluate(std::shared_ptr<Expr> expr) {
    return expr->accept(this);
  }
  auto isTruthy(typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return false;
    if (std::holds_alternative<bool>(obj)) return std::get<bool>(obj);
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
  void executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements, std::shared_ptr<Env> env) {
    BlockExecution { this, statements, this->env, env, }.execute();
  }
  typeRuntimeValue visitBinaryExpr(const BinaryExpr* expr) override {
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
  typeRuntimeValue visitLiteralExpr(const LiteralExpr* expr) override {
    // Return as runtime values.
    return expr->value;  
  }
  typeRuntimeValue visitGroupingExpr(const GroupingExpr* expr) override {
    // Recursively evaluate that subexpression and return it.
    return evaluate(expr->expression);  
  }
  typeRuntimeValue visitUnaryExpr(const UnaryExpr* expr) override {
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
  typeRuntimeValue visitAssignExpr(const AssignExpr* expr) override {
    auto value = evaluate(expr->value);
    env->assign(expr->name, value);
    return value;
  }
  typeRuntimeValue visitVariableExpr(const VariableExpr* expr) override {
    return env->get(expr->name);
  }
  void visitExpressionStmt(const ExpressionStmt* stmt) override {
    evaluate(stmt->expression);
  }
  void visitPrintStmt(const PrintStmt* stmt) override {
    std::cout << toRawString(stringifyVariantValue(evaluate(stmt->expression)));
  }
  void visitVarStmt(const VarStmt* stmt) override {
    typeRuntimeValue value;
    if (stmt->initializer != nullptr) {
      value = evaluate(stmt->initializer);
    }
    env->define(stmt->name.lexeme, value);
  }
  void visitBlockStmt(const BlockStmt* stmt) override {
    executeBlock(stmt->statements, std::make_shared<Env>(env));
  }
  void execute(std::shared_ptr<Stmt> stmt) {
    stmt->accept(this);
  }
 public:
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

#endif
