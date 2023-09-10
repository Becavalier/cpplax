#ifndef	_RESOLVER_H
#define	_RESOLVER_H

#include <memory>
#include <vector>
#include <string>
#include "./expr.h"
#include "./stmt.h"
#include "./interpreter.h"
#include "./env.h"
#include "./error.h"
#include "./type.h"

/**
 * Visit every node in the AST to do the variable resolution.
 * 
 * - BlockStmt: create new scope.
 * - FunctionStmt: create new scope, add variables (parameters).
 * - VarStmt: add variables.
 * - VariableExpr: bind local.
 * - AssignExpr: bind local (lvalue).
 * 
*/
struct Resolver : public ExprVisitor, public StmtVisitor {
  Interpreter* interpreter;
  FunctionType currentFunction = FunctionType::NONE;
  std::vector<typeScopeRecord> scopes {};  // For tracking local block scopes.
  explicit Resolver(Interpreter* interpreter) : interpreter(interpreter) {}
  void resolve(Stmt::sharedStmtPtr stmt) {
    stmt->accept(this);
  } 
  void resolve(Expr::sharedExprPtr expr) {
    expr->accept(this);
  }
  void resolve(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
      resolve(statement);
    }
  }
  void beginScope(void) {
    scopes.push_back(typeScopeRecord {});
  }
  void endScope(void) {
    scopes.pop_back();
  }
  void declare(const Token& name) {
    // Add the variable to the innermost scope so that it shadows any outer one.
    if (scopes.empty()) return;
    auto& scope = scopes.back();
    if (scope.contains(name.lexeme)) {
      Error::error(name, "already a variable with this name in this scope.");
    }
    scope[name.lexeme] = false;  // Mark as "not ready yet".
  }
  void define(const Token& name) {
    if (scopes.empty()) return;
    scopes.back()[name.lexeme] = true;  // Mark as "fully initialized".
  }
  void resolveLocal(Expr::sharedConstExprPtr expr, const Token& name) {
    // Start at the innermost scope and work outwards, looking in each map for a matching name.
    for (auto i = scopes.size(); i >= 1; i--) {
      if (scopes.at(i - 1).contains(name.lexeme)) {
        interpreter->resolve(expr, scopes.size() - i);
      }
    }
  }
  void resolveFunction(std::shared_ptr<const FunctionStmt> function, FunctionType type) {
    auto enclosingFunction = currentFunction;  // Stash the previous value.
    currentFunction = type;
    beginScope();
    for (const auto& parame : function->parames) {
      declare(parame);
      define(parame);
    }
    resolve(function->body);
    endScope();
    currentFunction = enclosingFunction;
  }
  void visitBlockStmt(std::shared_ptr<const BlockStmt> stmt) override {
    beginScope();
    resolve(stmt->statements);
    endScope();
  }
  void visitVarStmt(std::shared_ptr<const VarStmt> stmt) override {
    /**
     * For case like below:
     * 
     * var a = "outer";
     * {
     *   var a = a;
     * }
     * 
     * Make it an error to reference a variable in its initializer.
    */
    declare(stmt->name);
    if (stmt->initializer != nullptr) {
      resolve(stmt->initializer);
    }
    define(stmt->name);
  }
  void visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) override {
    // Enable recursion.
    declare(stmt->name);
    define(stmt->name);
    resolveFunction(stmt, FunctionType::FUNCTION);
  };
  void visitExpressionStmt(std::shared_ptr<const ExpressionStmt> stmt) override {
    resolve(stmt->expression);
  }
  void visitIfStmt(std::shared_ptr<const IfStmt> stmt) override {
    resolve(stmt->condition);
    resolve(stmt->thenBranch);
    if (stmt->elseBranch != nullptr) resolve(stmt->elseBranch);
  }
  void visitPrintStmt(std::shared_ptr<const PrintStmt> stmt) override {
    resolve(stmt->expression);
  }
  void visitReturnStmt(std::shared_ptr<const ReturnStmt> stmt) override {
    if (currentFunction == FunctionType::NONE) {
      Error::error(stmt->keyword, "can't return from top-level code.");
    }
    if (stmt->value != nullptr) {
      resolve(stmt->value);
    }
  }
  void visitWhileStmt(std::shared_ptr<const WhileStmt> stmt) override {
    resolve(stmt->condition);
    resolve(stmt->body);
  }
  typeRuntimeValue visitVariableExpr(std::shared_ptr<const VariableExpr> expr) override { 
    // Check if the variable is being accessed inside its own initializer.
    if (!scopes.empty()) {
      const auto& exist = scopes.back().find(expr->name.lexeme);
      if (exist != scopes.back().end() && !exist->second) {
        Error::error(expr->name, "can't read local variable in its own initializer.");
      }
    }
    resolveLocal(expr, expr->name);
    return std::monostate {};
  }
  typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr> expr) override {
    resolve(expr->value);
    resolveLocal(expr, expr->name);
    return std::monostate {};
  }
  typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr> expr) override {
    resolve(expr->left);
    resolve(expr->right);
    return std::monostate {};
  }
  typeRuntimeValue visitCallExpr(std::shared_ptr<const CallExpr> expr) override {
    resolve(expr->callee);
    for (const auto& argument : expr->arguments) {
      resolve(argument);
    }
    return std::monostate {};
  }
  typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr> expr) override { 
    resolve(expr->expression);
    return std::monostate {};
  }  
  typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr>) override {
    return std::monostate {};
  }
  typeRuntimeValue visitLogicalExpr(std::shared_ptr<const LogicalExpr> expr) override {
    resolve(expr->left);
    resolve(expr->right);
    return std::monostate {};
  }
  typeRuntimeValue visitUnaryExpr(std::shared_ptr<const UnaryExpr> expr) override {
    resolve(expr->right);
    return std::monostate {};
  }
};

#endif