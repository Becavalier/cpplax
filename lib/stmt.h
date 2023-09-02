#ifndef	_STMT_H
#define	_STMT_H

#include <memory>
#include <vector>
#include "lib/type.h"
#include "lib/expr.h"

struct ExpressionStmt;
struct PrintStmt;
struct VarStmt;
struct BlockStmt;

struct StmtVisitor {
  virtual void visitExpressionStmt(const ExpressionStmt*) = 0;
  virtual void visitPrintStmt(const PrintStmt*) = 0;
  virtual void visitVarStmt(const VarStmt*) = 0;
  virtual void visitBlockStmt(const BlockStmt*) = 0;
};


struct Stmt {
  virtual void accept(StmtVisitor* visitor) = 0;
  virtual ~Stmt() {};
};

struct ExpressionStmt : public Stmt {
  const std::shared_ptr<Expr> expression;
  ExpressionStmt(std::shared_ptr<Expr> expression) : expression(expression) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitExpressionStmt(this);
  }
};

struct PrintStmt : public Stmt {
  const std::shared_ptr<Expr> expression;
  PrintStmt(std::shared_ptr<Expr> expression) : expression(expression) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitPrintStmt(this);
  }
};

struct VarStmt : public Stmt {
  const Token& name;
  const std::shared_ptr<Expr> initializer;
  VarStmt(const Token& name, std::shared_ptr<Expr> initializer) : name(name), initializer(initializer) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitVarStmt(this);
  }
};

struct BlockStmt : public Stmt {
  std::vector<std::shared_ptr<Stmt>> statements;
  BlockStmt(std::vector<std::shared_ptr<Stmt>> statements) : statements(statements) {}
  void accept(StmtVisitor* visitor) override {
    visitor->visitBlockStmt(this);
  }
};


#endif
