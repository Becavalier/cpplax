#ifndef	_STMT_H
#define	_STMT_H

#include <memory>
#include <vector>
#include <functional>
#include "./type.h"
#include "./expr.h"

struct ExpressionStmt;
struct VarStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct FunctionStmt;
struct ReturnStmt;
struct ClassStmt;

struct StmtVisitor {
  virtual void visitExpressionStmt(std::shared_ptr<const ExpressionStmt>) = 0;
  virtual void visitVarStmt(std::shared_ptr<const VarStmt>) = 0;
  virtual void visitBlockStmt(std::shared_ptr<const BlockStmt>) = 0;
  virtual void visitIfStmt(std::shared_ptr<const IfStmt>) = 0;
  virtual void visitWhileStmt(std::shared_ptr<const WhileStmt>) = 0;
  virtual void visitFunctionStmt(std::shared_ptr<const FunctionStmt>) = 0;
  virtual void visitReturnStmt(std::shared_ptr<const ReturnStmt>) = 0;
  virtual void visitClassStmt(std::shared_ptr<const ClassStmt>) = 0;
  virtual ~StmtVisitor() {}
};


struct Stmt {
  using sharedStmtPtr = std::shared_ptr<Stmt>;
  virtual void accept(StmtVisitor*) = 0;
  virtual ~Stmt() {};
};

struct ExpressionStmt : public Stmt, public std::enable_shared_from_this<ExpressionStmt> {
  const Expr::sharedExprPtr expression;
  explicit ExpressionStmt(Expr::sharedExprPtr expression) : expression(expression) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitExpressionStmt(shared_from_this());
  }
};

struct VarStmt : public Stmt, public std::enable_shared_from_this<VarStmt> {
  const Token& name;
  const Expr::sharedExprPtr initializer;
  VarStmt(const Token& name, Expr::sharedExprPtr initializer) : name(name), initializer(initializer) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitVarStmt(shared_from_this());
  }
};

struct BlockStmt : public Stmt, public std::enable_shared_from_this<BlockStmt> {
  std::vector<sharedStmtPtr> statements;  // Make it mtable for supporting syntax sugar.
  explicit BlockStmt(const std::vector<sharedStmtPtr>& statements) : statements(statements) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitBlockStmt(shared_from_this());
  }
};

struct IfStmt : public Stmt, public std::enable_shared_from_this<IfStmt> {
  const Expr::sharedExprPtr condition;
  const sharedStmtPtr thenBranch;
  const sharedStmtPtr elseBranch;
  IfStmt(Expr::sharedExprPtr condition, sharedStmtPtr thenBranch, sharedStmtPtr elseBranch) : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitIfStmt(shared_from_this());
  }
};

struct WhileStmt : public Stmt, public std::enable_shared_from_this<WhileStmt> {
  const Expr::sharedExprPtr condition;
  const sharedStmtPtr body;
  WhileStmt(Expr::sharedExprPtr condition, sharedStmtPtr body) : condition(condition), body(body) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitWhileStmt(shared_from_this());
  }
};

struct FunctionStmt : public Stmt, public std::enable_shared_from_this<FunctionStmt> {
  const Token& name;  // Function name.
  const std::vector<std::reference_wrapper<const Token>> parames;
  const std::vector<sharedStmtPtr> body;
  FunctionStmt(const Token& name, const std::vector<std::reference_wrapper<const Token>>& parames, const std::vector<sharedStmtPtr>& body) : name(name), parames(parames), body(body) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitFunctionStmt(shared_from_this());
  }
};

struct ReturnStmt : public Stmt, public std::enable_shared_from_this<ReturnStmt> {
  const Token& keyword;  // Save the "return" location.
  const Expr::sharedExprPtr value;
  ReturnStmt(const Token& keyword, const Expr::sharedExprPtr value) : keyword(keyword), value(value) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitReturnStmt(shared_from_this());
  }
};

struct ClassStmt : public Stmt, public std::enable_shared_from_this<ClassStmt> {
  const Token& name;  // Class name.
  const std::vector<std::shared_ptr<FunctionStmt>> methods;  // Methods (name, parameter list, body).
  const std::shared_ptr<VariableExpr> superClass;  // As a "VariableExpr", which would be resolved by resolver.
  ClassStmt(const Token& name, const std::vector<std::shared_ptr<FunctionStmt>>& methods, std::shared_ptr<VariableExpr> superClass) : name(name), methods(methods), superClass(superClass) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitClassStmt(shared_from_this());
  }
};


#endif
