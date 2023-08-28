#ifndef	_EXPR_H
#define	_EXPR_H

#include <string>
#include <any>
#include <sstream>
#include "lib/token.h"

// Symbols.
struct BinaryExpr;
struct LiteralExpr;
struct GroupingExpr;
struct AssignExpr;
struct CallExpr;
struct GetExpr;
struct LogicalExpr;
struct SetExpr;
struct SuperExpr;
struct ThisExpr;
struct VariableExpr;
struct UnaryExpr;

struct Visitor {
  // virtual std::any visitAssignExpr(const AssignExpr* expr) = 0;
  virtual std::any visitBinaryExpr(const BinaryExpr* expr) = 0;
  // virtual std::any visitCallExpr(const CallExpr* expr) = 0;
  // virtual std::any visitGetExpr(const GetExpr* expr) = 0;
  virtual std::any visitGroupingExpr(const GroupingExpr* expr) = 0;
  virtual std::any visitLiteralExpr(const LiteralExpr* expr) = 0;
  // virtual std::any visitLogicalExpr(const LogicalExpr* expr) = 0;
  // virtual std::any visitSetExpr(const SetExpr* expr) = 0;
  // virtual std::any visitSuperExpr(const SuperExpr* expr) = 0;
  // virtual std::any visitThisExpr(const ThisExpr* expr) = 0;
  virtual std::any visitUnaryExpr(const UnaryExpr* expr) = 0;
  // virtual std::any visitVariableExpr(const VariableExpr* expr) = 0;
};

struct Expr {
  virtual std::any accept(Visitor* visitor) const = 0;
  Expr() = default;
};

// struct AssignExpr : public Expr {
//   const Token& name;
//   const Expr& value;
//   AssignExpr(const Token& name, const Expr& value) : name(name), value(value) {}
//   std::any accept(Visitor* visitor) const override {
//     return visitor->visitAssignExpr(this);
//   }
// };

// struct CallExpr : public Expr {

// };

// struct GetExpr : public Expr {

// };

// struct LogicalExpr : public Expr {

// };

// struct SetExpr : public Expr {

// };

// struct SuperExpr : public Expr {

// };

// struct ThisExpr : public Expr {

// };

// struct VariableExpr : public Expr {

// };

struct BinaryExpr : public Expr {
  const Expr& left;
  const Token& op;
  const Expr& right;
  BinaryExpr(const Expr& left, const Token& op, const Expr& right) : left(left), op(op), right(right) {}
  std::any accept(Visitor* visitor) const override {
    return visitor->visitBinaryExpr(this);
  }
};

struct LiteralExpr : public Expr {
  const Token::typeLiteral& value;
  LiteralExpr(const Token::typeLiteral& value) : value(value) {}
  std::any accept(Visitor* visitor) const override {
    return visitor->visitLiteralExpr(this);
  }
};

struct GroupingExpr : public Expr {
  const Expr& expression;
  GroupingExpr(const Expr& expression) : expression(expression) {}
  std::any accept(Visitor* visitor) const override {
    return visitor->visitGroupingExpr(this);
  }
};

struct UnaryExpr : public Expr {
  const Token& op;
  const Expr& right;
  UnaryExpr(const Token& op, const Expr& right) : op(op), right(right) {}
  std::any accept(Visitor* visitor) const override {
    return visitor->visitUnaryExpr(this);
  }
};

class AstPrinter : public Visitor {
  template<typename... Ts>
  std::string parenthesize(const std::string& name, const Ts&... args) {
    std::stringstream ss;
    ss << '(' << name;
    auto _expandArgPack = [&](const auto& arg) -> void {
      ss << ' ' << std::any_cast<std::string const&>(arg.accept(this));
    };
    (_expandArgPack(args), ...);
    ss << ')';
    return ss.str();
  }
  std::any visitBinaryExpr(const BinaryExpr* expr) override {
    return parenthesize(expr->op.lexeme, expr->left, expr->right);
  }
  std::any visitLiteralExpr(const LiteralExpr* expr) override {
    return Token::stringifyLiteralValue(expr->value);
  }
  std::any visitGroupingExpr(const GroupingExpr* expr) override {
    return parenthesize("group", expr->expression);
  }
  std::any visitUnaryExpr(const UnaryExpr* expr) override {
    return parenthesize(expr->op.lexeme, expr->right);
  }
 public:
  std::any print(Expr& expr) {
    return expr.accept(this);
  }
};

#endif
