#ifndef	_EXPR_H
#define	_EXPR_H

#include <string>
#include <sstream>
#include <memory>
#include <variant>
#include "./token.h"
#include "./helper.h"
#include "./type.h"

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

struct ExprVisitor {
  virtual typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr>) = 0;
  virtual typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr>) = 0;
  virtual typeRuntimeValue visitCallExpr(std::shared_ptr<const CallExpr>) = 0;
  // virtual typeRuntimeValue visitGetExpr(const GetExpr* expr) = 0;
  virtual typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr>) = 0;
  virtual typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr>) = 0;
  virtual typeRuntimeValue visitLogicalExpr(std::shared_ptr<const LogicalExpr>) = 0;
  // virtual typeRuntimeValue visitSetExpr(const SetExpr* expr) = 0;
  // virtual typeRuntimeValue visitSuperExpr(const SuperExpr* expr) = 0;
  // virtual typeRuntimeValue visitThisExpr(const ThisExpr* expr) = 0;
  virtual typeRuntimeValue visitUnaryExpr(std::shared_ptr<const UnaryExpr>) = 0;
  virtual typeRuntimeValue visitVariableExpr(std::shared_ptr<const VariableExpr>) = 0;
  virtual ~ExprVisitor() {}
};

struct Expr {
  using sharedExprPtr = std::shared_ptr<Expr>;
  using sharedConstExprPtr = std::shared_ptr<const Expr>;
  virtual typeRuntimeValue accept(ExprVisitor*) = 0;
  virtual ~Expr() {};
};

struct AssignExpr : public Expr, public std::enable_shared_from_this<AssignExpr> {
  const Token& name;  // The variable being assigned to.
  const sharedExprPtr value;
  AssignExpr(const Token& name, sharedExprPtr value) : name(name), value(value) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitAssignExpr(shared_from_this());
  }
};

struct CallExpr : public Expr, public std::enable_shared_from_this<CallExpr> {
  const sharedExprPtr callee;  // Could be a "CallExpr" as well.
  const Token& paren;  // Record the location of the closing parenthesis.
  const std::vector<sharedExprPtr> arguments;
  CallExpr(sharedExprPtr callee, const Token& paren, const std::vector<sharedExprPtr>& arguments) : callee(callee), paren(paren), arguments(arguments) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitCallExpr(shared_from_this());
  }
};

// struct GetExpr : public Expr {

// };

struct LogicalExpr : 
  public Expr, 
  public std::enable_shared_from_this<LogicalExpr>{
  const sharedExprPtr left;
  const Token& op;
  const sharedExprPtr right;
  LogicalExpr(sharedExprPtr left, const Token& op, sharedExprPtr right) : left(left), op(op), right(right) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitLogicalExpr(shared_from_this());
  }
};

// struct SetExpr : public Expr {

// };

// struct SuperExpr : public Expr {

// };

// struct ThisExpr : public Expr {

// };

struct VariableExpr : public Expr, public std::enable_shared_from_this<VariableExpr> {
  const Token& name;
  explicit VariableExpr(const Token& name) : name(name) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitVariableExpr(shared_from_this());
  }
};

struct BinaryExpr : public Expr, public std::enable_shared_from_this<BinaryExpr> {
  const Token& op;
  const sharedExprPtr left;
  const sharedExprPtr right;
  BinaryExpr(sharedExprPtr left, const Token& op, sharedExprPtr right) 
    : op(op), left(left), right(right) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitBinaryExpr(shared_from_this());
  }
};

struct LiteralExpr :  public Expr, public std::enable_shared_from_this<LiteralExpr> {
  const Token::typeLiteral value;
  explicit LiteralExpr(Token::typeLiteral value) : value(value) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitLiteralExpr(shared_from_this());
  }
};
 
struct GroupingExpr : public Expr, public std::enable_shared_from_this<GroupingExpr> {
  const sharedExprPtr expression;
  explicit GroupingExpr(sharedExprPtr expression) : expression(expression) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitGroupingExpr(shared_from_this());
  }
};

struct UnaryExpr : public Expr, public std::enable_shared_from_this<UnaryExpr> {
  const Token& op;
  const sharedExprPtr right;
  UnaryExpr(const Token& op, sharedExprPtr right) : op(op), right(right) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitUnaryExpr(shared_from_this());
  }
};

// Common AST traverser.
class ExprPrinter : public ExprVisitor, public std::enable_shared_from_this<ExprPrinter> {
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
    std::ostringstream oss;
    oss << '(' << name;
    ([&](const auto& arg) -> void {
      oss << ' ' << std::get<std::string>(arg->accept(this));
    }(args), ...);
    oss << ')';
    return oss.str();
  }
  typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr> expr) override {
    return parenthesize(expr->op.lexeme, expr->left, expr->right);
  }
  typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr> expr) override {
    return stringifyVariantValue(expr->value);
  }
  typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr> expr) override {
    return parenthesize("group", expr->expression);
  }
  typeRuntimeValue visitUnaryExpr(std::shared_ptr<const UnaryExpr> expr) override {
    return parenthesize(expr->op.lexeme, expr->right);
  }
 public:
  void print(Expr::sharedExprPtr expr) {
    const auto str = std::get<std::string>(expr->accept(this));
    formatOutputAST(str);
  }
};

#endif
