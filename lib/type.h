#ifndef	_TYPE_H
#define	_TYPE_H

/**
 * Basic types for both compiler and interpreter.
*/

#include <variant>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "./common.h"

using typeRuntimeNumericValue= double;

enum TokenType : uint8_t {
  // Single-character tokens.
  LEFT_PAREN, 
  RIGHT_PAREN, 
  LEFT_BRACE, 
  RIGHT_BRACE,
  COMMA, 
  DOT, 
  MINUS, 
  PLUS, 
  SEMICOLON, 
  SLASH, 
  STAR,

  // One or two character tokens.
  BANG, 
  BANG_EQUAL,
  EQUAL, 
  EQUAL_EQUAL,
  GREATER, 
  GREATER_EQUAL,
  LESS, 
  LESS_EQUAL,

  // Literals.
  IDENTIFIER, 
  STRING, 
  NUMBER,

  // Keywords.
  AND, 
  CLASS, 
  ELSE, 
  FALSE, 
  FN, 
  FOR, 
  IF, 
  NIL, 
  OR,
  RETURN, 
  SUPER, 
  THIS, 
  TRUE, 
  VAR, 
  WHILE, 
  SOURCE_EOF,
  TOTAL,
};


// Forward declarations.
struct ClassInstance;
struct Invokable;
struct Obj;
using typeRuntimeValue = 
  std::variant<
    // Common fields.
    std::monostate, 
    std::string_view, 
    typeRuntimeNumericValue, 
    bool,
    // Interpreter fields.
    std::shared_ptr<Invokable>, 
    std::shared_ptr<ClassInstance>,
    std::string,
    // VM & Compiler fields.
    Obj*  // Pointer to the heap value.
  >;

// Class "Invokable", for function and method.
struct Interpreter;
struct Invokable {  
  virtual std::string toString(void) = 0;
  virtual size_t arity() = 0;
  virtual typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) = 0;
  virtual ~Invokable() {}
};

/**
 * Class "Function".
*/
struct Env;
struct FunctionStmt;
struct Function : public Invokable {
  const std::shared_ptr<const FunctionStmt> declaration;
  std::shared_ptr<Env> closure;  // Capture the env at the definition place.
  bool isInitializer;
  Function(std::shared_ptr<const FunctionStmt> declaration, std::shared_ptr<Env> closure, bool isInitializer) : declaration(declaration), closure(closure), isInitializer(isInitializer) {}
  std::string toString(void) override;
  size_t arity() override;
  typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override;
  std::shared_ptr<Function> bind(std::shared_ptr<ClassInstance>);
};

/**
 * Class "Class".
 * 
 * Class() -> 
 * ClassInstance -> 
 * (GetExpr / SetExpr) -> method / field.
*/
struct Class : public Invokable, public std::enable_shared_from_this<Class> {
  const std::string_view name;
  std::shared_ptr<Class> superClass;
  std::shared_ptr<Function> initializer;
  std::unordered_map<std::string_view, std::shared_ptr<Function>> methods;
  explicit Class(
    const std::string_view name, 
    std::shared_ptr<Class> superClass,
    std::unordered_map<std::string_view, std::shared_ptr<Function>>& methods) 
    : name(name), superClass(superClass), methods(methods) {
      initializer = findMethod(INITIALIZER_NAME);
    }
  std::string toString(void) override;
  size_t arity() override;
  typeRuntimeValue invoke(Interpreter*, std::vector<typeRuntimeValue>&) override;
  std::shared_ptr<Function> findMethod(const std::string_view);
};


/**
 * Class "ClassInstance".
 * The runtime representation of the Class instance.
*/
struct Token;
struct ClassInstance : public std::enable_shared_from_this<ClassInstance> {
  std::shared_ptr<Class> thisClass;
  std::unordered_map<std::string_view, typeRuntimeValue> fields;  // Runtime state of each instance.
  explicit ClassInstance(std::shared_ptr<Class> thisClass) : thisClass(thisClass) {}
  std::string toString(void);
  typeRuntimeValue get(const Token&);
  void set(const Token&, typeRuntimeValue);
};


/**
 * Interpreter Related Types
*/
enum class FunctionType : uint8_t {
  NONE,
  FUNCTION,
  INITIALIZER,
  METHOD,
};

enum class ClassType : uint8_t {
  NONE,
  CLASS,
  SUBCLASS,
};


using typeKeywordList = std::unordered_map<std::string_view, TokenType>;
using typeScopeRecord = std::unordered_map<std::string_view, bool>;

/**
 * VM & Compiler Related Types
*/
using OpCodeType = uint8_t;
using typeVMCodeArray = std::vector<uint8_t>;
enum OpCode : OpCodeType {
  OP_CONSTANT,  // [OpCode, Constant Index (uint8_t)].
  // OP_CONSTANT_LONG,
  OP_RETURN, 
  OP_NEGATE, 
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_NOT,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_POP, 
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_JUMP_IF_FALSE,  // [OpCode, offset].
  OP_JUMP,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_CLOSE_UPVALUE,
  OP_CLASS,
  OP_SET_PROPERTY,
  OP_GET_PROPERTY,
  OP_METHOD,
  OP_INVOKE,
  OP_INHERIT,
  OP_GET_SUPER,
  OP_SUPER_INVOKE,
};

enum class VMResult : uint8_t {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

enum class ObjType : uint8_t {
  OBJ_FUNCTION,
  OBJ_STRING,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_CLASS,
  OBJ_INSTANCE,
  OBJ_BOUND_METHOD,
};

enum class FunctionScope : uint8_t {
  TYPE_TOP_LEVEL,
  TYPE_BODY,
  TYPE_METHOD,
  TYPE_INITIALIZER,
};

using typeRuntimeConstantArray = std::vector<typeRuntimeValue>;
using typeVMStack = std::array<typeRuntimeValue, STACK_MAX>;
template<typename T = typeRuntimeValue> 
using typeVMStore = std::unordered_map<Obj*, T>;

#endif
