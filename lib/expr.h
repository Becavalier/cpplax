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
struct GetExpr;
struct ExprVisitor {
  virtual typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr>) = 0;
  virtual typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr>) = 0;
  virtual typeRuntimeValue visitCallExpr(std::shared_ptr<const CallExpr>) = 0;
  virtual typeRuntimeValue visitGetExpr(std::shared_ptr<const GetExpr>) = 0;
  virtual typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr>) = 0;
  virtual typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr>) = 0;
  virtual typeRuntimeValue visitLogicalExpr(std::shared_ptr<const LogicalExpr>) = 0;
  virtual typeRuntimeValue visitSetExpr(std::shared_ptr<const SetExpr>) = 0;
  virtual typeRuntimeValue visitSuperExpr(std::shared_ptr<const SuperExpr>) = 0;
  virtual typeRuntimeValue visitThisExpr(std::shared_ptr<const ThisExpr>) = 0;
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

struct LogicalExpr : public Expr, public std::enable_shared_from_this<LogicalExpr>{
  const sharedExprPtr left;
  const Token& op;
  const sharedExprPtr right;
  LogicalExpr(sharedExprPtr left, const Token& op, sharedExprPtr right) : left(left), op(op), right(right) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitLogicalExpr(shared_from_this());
  }
};

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
  const typeRuntimeValue value;
  explicit LiteralExpr(typeRuntimeValue value) : value(value) {}
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

struct GetExpr : public Expr, public std::enable_shared_from_this<GetExpr> {
  const Token& name;  // The property name that is being accessed (identifier).
  const sharedExprPtr obj;  // The callee (primary).
  GetExpr(const Token& name, sharedExprPtr obj) : name(name), obj(obj) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitGetExpr(shared_from_this());
  }
};

struct SetExpr : public Expr, public std::enable_shared_from_this<SetExpr> {
  const sharedExprPtr obj; 
  const Token& name;
  const sharedExprPtr value; 
  SetExpr(sharedExprPtr obj, const Token& name, sharedExprPtr value) : obj(obj), name(name), value(value) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitSetExpr(shared_from_this());
  }
};

struct ThisExpr : public Expr, public std::enable_shared_from_this<ThisExpr> {
  const Token& keyword; 
  explicit ThisExpr(const Token& keyword) : keyword(keyword) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitThisExpr(shared_from_this());
  }
};

struct SuperExpr : public Expr, public std::enable_shared_from_this<SuperExpr> {
  const Token& keyword; 
  const Token& method; 
  explicit SuperExpr(const Token& keyword, const Token& method) : keyword(keyword), method(method) {}
  typeRuntimeValue accept(ExprVisitor* visitor) override {
    return visitor->visitSuperExpr(shared_from_this());
  }
};

#endif
