#ifndef	_TYPE_H
#define	_TYPE_H

#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

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
using typeKeywordList = std::unordered_map<std::string_view, TokenType>;
using typeScopeRecord = std::unordered_map<std::string_view, bool>;

/**
 * VM Related Types
*/
using OpCodeType = uint8_t;
enum OpCode : OpCodeType {
  OP_CONSTANT,  // [OpCode, Constant Index (uint8_t)].
  // OP_CONSTANT_LONG,
  OP_RETURN,  // [OpCode].
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
};

enum class VMResult : uint8_t {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

using typeVMCodeArray = std::vector<uint8_t>;
using typeRuntimeNumericValueArray = std::vector<typeRuntimeNumericValue>;

// Runtime value.
using typeRuntimeValue = std::variant<
  std::monostate, 
  std::shared_ptr<Invokable>, 
  std::shared_ptr<ClassInstance>,
  std::string_view, 
  typeRuntimeNumericValue, 
  bool>;

#endif
