#ifndef	_EXPR_H
#define	_EXPR_H

#include <string>
#include <sstream>
#include <memory>
#include <variant>
#include "lib/token.h"
#include "lib/helper.h"

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

using typeRuntimeValue = std::variant<
  std::monostate, 
  std::string, 
  double, 
  bool
>;
struct Visitor {
  // virtual typeRuntimeValue visitAssignExpr(const AssignExpr* expr) = 0;
  virtual typeRuntimeValue visitBinaryExpr(const BinaryExpr* expr) = 0;
  // virtual typeRuntimeValue visitCallExpr(const CallExpr* expr) = 0;
  // virtual typeRuntimeValue visitGetExpr(const GetExpr* expr) = 0;
  virtual typeRuntimeValue visitGroupingExpr(const GroupingExpr* expr) = 0;
  virtual typeRuntimeValue visitLiteralExpr(const LiteralExpr* expr) = 0;
  // virtual typeRuntimeValue visitLogicalExpr(const LogicalExpr* expr) = 0;
  // virtual typeRuntimeValue visitSetExpr(const SetExpr* expr) = 0;
  // virtual typeRuntimeValue visitSuperExpr(const SuperExpr* expr) = 0;
  // virtual typeRuntimeValue visitThisExpr(const ThisExpr* expr) = 0;
  virtual typeRuntimeValue visitUnaryExpr(const UnaryExpr* expr) = 0;
  // virtual typeRuntimeValue visitVariableExpr(const VariableExpr* expr) = 0;
};

struct Expr {
  virtual typeRuntimeValue accept(Visitor* visitor) = 0;
  virtual ~Expr() {};
};

// struct AssignExpr : public Expr {
//   const Token& name;
//   const Expr& value;
//   AssignExpr(const Token& name, const Expr& value) : name(name), value(value) {}
//   typeRuntimeValue accept(Visitor* visitor) const override {
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
  const Token& op;
  const std::shared_ptr<Expr> left;
  const std::shared_ptr<Expr> right;
  BinaryExpr(std::shared_ptr<Expr> left, const Token& op, std::shared_ptr<Expr> right) 
    : op(op), left(left), right(right) {}
  typeRuntimeValue accept(Visitor* visitor) override {
    return visitor->visitBinaryExpr(this);
  }
};

struct LiteralExpr : public Expr {
  const Token::typeLiteral value;
  LiteralExpr(const Token::typeLiteral value) : value(value) {}
  typeRuntimeValue accept(Visitor* visitor) override {
    return visitor->visitLiteralExpr(this);
  }
};

struct GroupingExpr : public Expr {
  const std::shared_ptr<Expr> expression;
  GroupingExpr(std::shared_ptr<Expr> expression) : expression(expression) {}
  typeRuntimeValue accept(Visitor* visitor) override {
    return visitor->visitGroupingExpr(this);
  }
};

struct UnaryExpr : public Expr {
  const Token& op;
  const std::shared_ptr<Expr> right;
  UnaryExpr(const Token& op, std::shared_ptr<Expr> right) : op(op), right(right) {}
  typeRuntimeValue accept(Visitor* visitor) override {
    return visitor->visitUnaryExpr(this);
  }
};

// Common AST traverser.
class AstPrinter : public Visitor {
  int indentCounter = 2;
  void formatOutputAST(const std::string& str) {
    auto curr = str.cbegin();
    while ((curr++) != str.cend()) {
      switch (*curr) {
        case '(': {
          std::cout << '\n';
          for (int i = indentCounter; i > 0; i--) std::cout << ' ';
          indentCounter += 2;
          break;
        }
      }
      std::cout << *curr;
    }
  }
  template<typename... Ts>
  std::string parenthesize(const std::string& name, const Ts&... args) {
    std::stringstream ss;
    ss << '(' << name;
    ([&](const auto& arg) -> void {
      ss << ' ' << std::get<std::string>(arg->accept(this));
    }(args), ...);
    ss << ')';
    return ss.str();
  }
  typeRuntimeValue visitBinaryExpr(const BinaryExpr* expr) override {
    return parenthesize(expr->op.lexeme, expr->left, expr->right);
  }
  typeRuntimeValue visitLiteralExpr(const LiteralExpr* expr) override {
    return stringifyVariantValue(expr->value);
  }
  typeRuntimeValue visitGroupingExpr(const GroupingExpr* expr) override {
    return parenthesize("group", expr->expression);
  }
  typeRuntimeValue visitUnaryExpr(const UnaryExpr* expr) override {
    return parenthesize(expr->op.lexeme, expr->right);
  }
 public:
  void print(std::shared_ptr<Expr> expr) {
    const auto str = std::get<std::string>(expr->accept(this));
    formatOutputAST(str);
  }
};

#endif
