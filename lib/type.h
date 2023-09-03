#ifndef	_TYPE_H
#define	_TYPE_H

#include <variant>
#include <string>
#include <memory>
#include <unordered_map>

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
  PRINT, 
  RETURN, 
  SUPER, 
  THIS, 
  TRUE, 
  VAR, 
  WHILE, 
  SOURCE_EOF
};

struct Invokable;
using typeRuntimeValue = std::variant<
  std::shared_ptr<Invokable>,
  std::monostate, 
  std::string, 
  double, 
  bool
>;

using typeKeywordList = std::unordered_map<std::string, TokenType>;

#endif
