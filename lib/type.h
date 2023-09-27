#ifndef	_TYPE_H
#define	_TYPE_H

#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

/**
 * Interpreter Related Types
*/
enum class TokenType : uint8_t {
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
  ERROR,
};

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
using typeRuntimeValue = std::variant<
  std::monostate, 
  std::shared_ptr<Invokable>, 
  std::shared_ptr<ClassInstance>,
  std::string_view, 
  double, 
  bool>;
using typeKeywordList = std::unordered_map<std::string_view, TokenType>;
using typeScopeRecord = std::unordered_map<std::string_view, bool>;

/**
 * VM Related Types
*/
enum OpCode : uint8_t {
  OP_CONSTANT,  // [OpCode, Constant Index (uint8_t)].
  // OP_CONSTANT_LONG,
  OP_RETURN,  // [OpCode].
  OP_NEGATE, 
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
};

enum VMResult : uint8_t {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

using typeVMCodeArray = std::vector<uint8_t>;
using typeVMValue = double;
using typeVMValueArray = std::vector<typeVMValue>;

#endif
