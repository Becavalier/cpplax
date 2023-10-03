#ifndef	_INTERPRETER_H
#define	_INTERPRETER_H

/**
 * This is the implementation of a AST tree-walking interpreter.
 * 
 * 1. Overhead is high due to the dynamic dispatch of each "visit method".
 * 2. Sprinkling data across the heap in a loosely connected web of objects does bad things for spatial locality.
 * 3. Not memory-efficient because of the way of AST nodes' representation in memory.
*/

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <variant>
#include "./expr.h"
#include "./helper.h"
#include "./error.h"
#include "./stmt.h"
#include "./helper.h"
#include "./env.h"
#include "./type.h"

// Forward declarations.
struct Interpreter;
struct ClassInstance;

/**
 * Class "Invokable", for function and method.
*/
struct Invokable {  
  virtual const char* toString(void) = 0;
  virtual size_t arity() = 0;
  virtual typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) = 0;
  virtual ~Invokable() {}
};

/**
 * Class "Function".
*/
struct Function : public Invokable {
  Function(std::shared_ptr<const FunctionStmt> declaration, std::shared_ptr<Env> closure, bool isInitializer) : declaration(declaration), closure(closure), isInitializer(isInitializer) {}
  const char* toString(void) override { 
    return "<rt-core>"; 
  }
  size_t arity() override { 
    return declaration->parames.size(); 
  }
  typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override;
  std::shared_ptr<Function> bind(std::shared_ptr<ClassInstance> instance) {
    auto env = std::make_shared<Env>(closure);  // Create a new environment nestled inside the method’s original closure.
    env->define("this", instance);
    return std::make_shared<Function>(declaration, env, isInitializer);
  }
 private:
  const std::shared_ptr<const FunctionStmt> declaration;
  std::shared_ptr<Env> closure;  // Capture the env at the definition place.
  bool isInitializer;
};

/**
 * Class "Class".
 * 
 * Class() -> 
 * ClassInstance -> 
 * (GetExpr / SetExpr) -> method / field.
*/
struct Class : public Invokable, public std::enable_shared_from_this<Class> {
  explicit Class(
    const std::string_view name, 
    std::shared_ptr<Class> superClass,
    std::unordered_map<std::string_view, std::shared_ptr<Function>>& methods) 
    : name(name), superClass(superClass), methods(methods) {
      initializer = findMethod("init");
    }
  const char* toString(void) override { 
    return "<rt-core>"; 
  }
  size_t arity() override {
    return initializer == nullptr ? 0 : initializer->arity();
  }
  typeRuntimeValue invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) override {
    auto instance = std::make_shared<ClassInstance>(shared_from_this());  // Return a new instance.
    if (initializer != nullptr) {
      initializer->bind(instance)->invoke(interpreter, arguments);  // Invoke the constructor.
    }
    return instance;
  }
  std::shared_ptr<Function> findMethod(const std::string_view name) {
    if (methods.contains(name)) {
      return methods[name];
    }
    if (superClass != nullptr) {
      return superClass->findMethod(name);  // Look up from the inherited class.
    }
    return nullptr;
  }
 private:
  const std::string_view name;
  std::shared_ptr<Class> superClass;
  std::shared_ptr<Function> initializer;
  std::unordered_map<std::string_view, std::shared_ptr<Function>> methods;
};


/**
 * Class "ClassInstance".
 * The runtime representation of the Class instance.
*/
struct ClassInstance : public std::enable_shared_from_this<ClassInstance> {
  explicit ClassInstance(std::shared_ptr<Class> thisClass) : thisClass(thisClass) {}
  typeRuntimeValue get(const Token& name) {  
    if (fields.contains(name.lexeme)) {
      return fields[name.lexeme];  // Access fields in an instance.
    }
    const auto& method = thisClass->findMethod(name.lexeme);  // Otherwise, access class method (fields shadow methods).
    if (method != nullptr) {
      return method->bind(shared_from_this());  // Change method's closure and bind it to new scope.
    }
    throw InterpreterError { name, "undefined property '" + std::string { name.lexeme } + "'." };
  }
  void set(const Token& name, typeRuntimeValue value) {
    fields[name.lexeme] = value;
  }
 private:
  std::shared_ptr<Class> thisClass;
  std::unordered_map<std::string_view, typeRuntimeValue> fields;  // Runtime state of each instance.
};

struct ReturnException : public std::exception {
  const typeRuntimeValue value;
  explicit ReturnException(const typeRuntimeValue& value) : value(value) {}
};


/**
 * Class "Interpreter".
*/
struct Interpreter : public ExprVisitor, public StmtVisitor {
  const std::shared_ptr<Env> globals = std::make_shared<Env>();
  // Saving the scope steps for locals.
  std::unordered_map<Expr::sharedConstExprPtr, size_t> locals;  
  std::shared_ptr<Env> env = globals;  // Global scope.
  struct BlockExecution {
    Interpreter* thisPtr;
    const std::vector<Stmt::sharedStmtPtr>& statements;
    const std::shared_ptr<Env> previousEnv;
    const std::shared_ptr<Env> env;
    void execute(void) {
      thisPtr->env = env; // Set up new env for the new scope.
      for (const auto& statement : statements) {
        thisPtr->execute(statement);
      }
    }
    ~BlockExecution() {
      thisPtr->env = previousEnv;  // Restore the reference to the previous env.
    }
  };
  Interpreter() {
    // Define internal functions.
    class InternalFunClockDef : public Invokable {
      const char* toString(void) override { 
        return "<rt-native-fn>"; 
      }
      size_t arity() override { return 0; };
      typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override {
        return static_cast<double>(
          std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
          ).count()
        );
      };
    };
    class InternalFunPrintDef : public Invokable {
      const char* toString(void) override { 
        return "<rt-native-fn>"; 
      }
      size_t arity() override { return 1; };
      typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>& arguments) override {
        std::cout << unescapeStr(stringifyVariantValue(arguments.back()));
        return std::monostate {};
      };
    };
    globals->define("clock", std::make_shared<InternalFunClockDef>());
    globals->define("print", std::make_shared<InternalFunPrintDef>());
  }
  typeRuntimeValue evaluate(Expr::sharedExprPtr expr) {
    return expr->accept(this);
  }
  void resolve(Expr::sharedConstExprPtr expr, size_t depth) {
    locals[expr] = depth;  // Save the "Expr" as the key in case of any conflicts.
  }
  auto isTruthy(typeRuntimeValue obj) const {
    if (std::holds_alternative<std::monostate>(obj)) return false;
    if (std::holds_alternative<bool>(obj)) return std::get<bool>(obj);
    if (std::holds_alternative<std::string_view>(obj)) return std::get<std::string_view>(obj).size() > 0;
    if (std::holds_alternative<typeRuntimeNumericValue>(obj)) return !isDoubleEqual(std::get<typeRuntimeNumericValue>(obj), 0);
    return true;
  }
  void checkNumberOperand(const Token& op, const typeRuntimeValue& operand) const {
    if (std::holds_alternative<typeRuntimeNumericValue>(operand)) return;
    throw InterpreterError { op, "operand must be a number." };
  }
  void checkNumberOperands(const Token& op, const typeRuntimeValue& left, const typeRuntimeValue& right) const {
    if (std::holds_alternative<typeRuntimeNumericValue>(left) && std::holds_alternative<typeRuntimeNumericValue>(right)) return;
    throw InterpreterError { op, "operands must be numbers." };
  }
  typeRuntimeValue lookUpVariable(const Token& name, Expr::sharedConstExprPtr expr) {
    const auto distance = locals.find(expr);
    if (distance != locals.end()) {
      return env->getAt(distance->second, name.lexeme);
    } else {
      return globals->get(name);  // Throw if variable is undefined.
    }
  }
  typeRuntimeValue visitLiteralExpr(std::shared_ptr<const LiteralExpr> expr) override { 
    return expr->value;  // Return as runtime values.
  }  
  typeRuntimeValue visitGroupingExpr(std::shared_ptr<const GroupingExpr> expr) override { 
    return evaluate(expr->expression);  // Recursively evaluate its subexpression and return the result.
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
      default: ;
    }
    return std::monostate {};
  }
  typeRuntimeValue visitAssignExpr(std::shared_ptr<const AssignExpr> expr) override {
    auto value = evaluate(expr->value);
    auto distance = locals.find(expr);
    if (distance != locals.end()) {
      env->assignAt(distance->second, expr->name, value);
    } else {
      globals->assign(expr->name, value);
    }
    return value;
  }
  typeRuntimeValue visitVariableExpr(std::shared_ptr<const VariableExpr> expr) override { 
    return lookUpVariable(expr->name, expr);
  }
  typeRuntimeValue visitLogicalExpr(std::shared_ptr<const LogicalExpr> expr) override {
    auto left = evaluate(expr->left);
    if (expr->op.type == TokenType::OR) {  // Short-circuit.
      if (isTruthy(left)) return left;
    } else {
      if (!isTruthy(left)) return left;
    }
    return evaluate(expr->right);
  }
  typeRuntimeValue visitBinaryExpr(std::shared_ptr<const BinaryExpr> expr) override {
    /**
     * Another semantic choice: 
     * We could have instead specified that the left operand is checked, -
     * before even evaluating the right.
    */
    // Evaluate the operands in "left-to-right" order.
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
          if (std::holds_alternative<std::string_view>(right)) {
            return (std::ostringstream {} << std::get<double>(left) << std::get<std::string_view>(right)).str();
          }
        }
        if (std::holds_alternative<std::string_view>(left)) {
          if (std::holds_alternative<double>(right)) {
            return (std::ostringstream {} << std::get<std::string_view>(left) << std::get<double>(right)).str();
          }
          if (std::holds_alternative<std::string_view>(right)) {
            return (std::ostringstream {} << std::get<std::string_view>(left) << std::get<std::string_view>(right)).str();
          }
        }
        throw InterpreterError { expr->op, "operand must be type of number or string." };
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
      arguments.push_back(evaluate(argument));  // Save evaluated args.
    }
    if (!std::holds_alternative<std::shared_ptr<Invokable>>(callee)) {  // Check if it's callable.
      throw InterpreterError { expr->paren, "can only call functions and classes." };
    }
    const auto function = std::get<std::shared_ptr<Invokable>>(callee);
    if (arguments.size() != function->arity()) {  // Check if it matches the calling arity.
      throw InterpreterError { expr->paren, "expected " + std::to_string(function->arity()) + " arguments but got " + std::to_string( arguments.size()) + "." };
    }
    return function->invoke(this, arguments);
  }
  typeRuntimeValue visitGetExpr(std::shared_ptr<const GetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a getter.
    if (vp != nullptr) return (*vp)->get(expr->name);
    throw InterpreterError { expr->name, "only instances have properties." };
  }
  typeRuntimeValue visitSetExpr(std::shared_ptr<const SetExpr> expr) override {
    auto obj = evaluate(expr->obj);
    auto vp = std::get_if<std::shared_ptr<ClassInstance>>(&obj);  // Only "ClassInstance" is able to have a setter.
    if (vp == nullptr) throw InterpreterError { expr->name, "only instances have fields." };
    auto value = evaluate(expr->value);
    (*vp)->set(expr->name, value);
    return value;
  }
  typeRuntimeValue visitSuperExpr(std::shared_ptr<const SuperExpr> expr) override {
    const auto distance = locals[expr];
    auto superClass = std::static_pointer_cast<Class>(std::get<std::shared_ptr<Invokable>>(env->getAt(distance, "super")));
    auto thisInstance = std::get<std::shared_ptr<ClassInstance>>(env->getAt(distance - 1, "this"));
    auto method = superClass->findMethod(expr->method.lexeme);
    if (method == nullptr) {
      throw InterpreterError { expr->method, "undefined property '" + std::string { expr->method.lexeme } + "'." };
    }
    return method->bind(thisInstance);
  }
  typeRuntimeValue visitThisExpr(std::shared_ptr<const ThisExpr> expr) override {
    return lookUpVariable(expr->keyword, expr);
  }
  void visitExpressionStmt(std::shared_ptr<const ExpressionStmt> stmt) override {
    evaluate(stmt->expression);
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
    } else if (stmt->elseBranch != nullptr) {
      execute(stmt->elseBranch);
    }
  }
  void visitWhileStmt(std::shared_ptr<const WhileStmt> stmt) override {
    while (isTruthy(evaluate(stmt->condition))) {
      execute(stmt->body);
    }
  }
  void visitFunctionStmt(std::shared_ptr<const FunctionStmt> stmt) override {
    std::shared_ptr<Invokable> invoker = std::make_shared<Function>(stmt, env, false);  // Save closure (the definition scope) as well.
    env->define(stmt->name.lexeme, invoker); 
  };
  void visitReturnStmt(std::shared_ptr<const ReturnStmt> stmt) override {
    // Unwind the call stack by exception.
    typeRuntimeValue value = std::monostate {};
    if (stmt->value != nullptr) value = evaluate(stmt->value);
    throw ReturnException { value };
  }
  void visitClassStmt(std::shared_ptr<const ClassStmt> stmt) override {
    std::shared_ptr<Class> superClass = nullptr;
    if (stmt->superClass != nullptr) {
      const auto superClassVal = evaluate(stmt->superClass);
      const auto superClassPtr = std::get_if<std::shared_ptr<Invokable>>(&superClassVal);
      const auto invalidSuperErrorMsg = "super class must be a class.";
      if (superClassPtr == nullptr) {
        throw InterpreterError { stmt->superClass->name, invalidSuperErrorMsg };
      } else {
        superClass = std::dynamic_pointer_cast<Class>(*superClassPtr);
        if (superClass == nullptr) {
          throw InterpreterError { stmt->superClass->name, invalidSuperErrorMsg };
        }
      }
    }
    env->define(stmt->name.lexeme, std::monostate {});
    if (superClass != nullptr) {
      env = std::make_shared<Env>(env);
      env->define("super", superClass);  // Store the reference to super class.
    }
    std::unordered_map<std::string_view, std::shared_ptr<Function>> methods;
    for (const auto& method : stmt->methods) {
      auto function = std::make_shared<Function>(method, env, method->name.lexeme == "init");  // Capture the current env as closure, which includes the reference to super class.
      methods[method->name.lexeme] = function;
    }
    auto thisClass = std::make_shared<Class>(stmt->name.lexeme, superClass, methods);  // Store all methods (behavior) into Class.
    if (superClass != nullptr) {
      env = env->enclosing;  // Restore the env for storing class.
    }
    env->assign(stmt->name, thisClass);
  }
  void executeBlock(const std::vector<Stmt::sharedStmtPtr>& statements, std::shared_ptr<Env> env) {
    BlockExecution { this, statements, this->env, env, }.execute();
  }
  void execute(Stmt::sharedStmtPtr stmt) {
    stmt->accept(this);
  }
  void interpret(const std::vector<Stmt::sharedStmtPtr>& statements) {
    try {
      for (const auto& statement : statements) {
        execute(statement);
      }
    } catch (const InterpreterError& interpretError) {
      Error::interpretError(interpretError);
    }
  }
};

// Post definitions.
typeRuntimeValue Function::invoke(Interpreter* interpreter, std::vector<typeRuntimeValue>& arguments) {
  /**
   * env (created when invoked) -> 
   * closure (created when visiting function stmt, saved as an Invokable) -> 
   * global.
  */
  auto env = std::make_shared<Env>(closure);  // Each function gets its own environment where it stores those variables.
  for (size_t i = 0; i < declaration->parames.size(); i++) {
    env->define(declaration->parames.at(i).get().lexeme, arguments.at(i));  // Save the passing arguments into the env.
  }
  try {
    interpreter->executeBlock(declaration->body, env);  // Replace the Interpreter's env with the given one and then execute the code.
  } catch (const ReturnException& ret) {
    if (isInitializer) return closure->getAt(0, "this");
    return ret.value;
  }
  if (isInitializer) return closure->getAt(0, "this");
  return std::monostate {};
}

#endif
