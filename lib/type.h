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
  _ZOMBIE_,
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

struct Invokable;
struct ClassInstance;
struct Token;
struct Obj;
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
};

enum class VMResult : uint8_t {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

enum class ObjType : uint8_t {
  OBJ_FUNCTION,
  OBJ_STRING,
};

enum class FunctionScope : uint8_t {
  TYPE_BODY,
  TYPE_TOP_LEVEL,
};

/**
 * Core runtime types
*/
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
    // VM & Compiler fieldds.
    Obj*  // Pointer to the heap value.
  >;
using typeRuntimeValueArray = std::vector<typeRuntimeValue>;

#endif
