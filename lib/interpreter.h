#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

#include <memory>
#include "lib/expr.h"
#include "lib/helper.h"
#include "lib/error.h"

// The evaluation result should be Token::typeLiteral.
class Interpreter : public Visitor {
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
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
          return std::get<double>(left) + std::get<double>(right);
        }
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
          return std::get<std::string>(left) + std::get<std::string>(right);
        }
        throw RuntimeError { expr->op, "operands must be two numbers or two strings." };
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
 public:
  void interpret(std::shared_ptr<Expr> expr) {
    try {
      typeRuntimeValue value = evaluate(expr);
      std::cout << stringifyVariantValue(value) << std::endl;
    } catch (RuntimeError error) {
      Error::runtimeError(error);
    }
  }
};

#endif
