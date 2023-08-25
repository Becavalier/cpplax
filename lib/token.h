#ifndef	_TOKEN_H
#define	_TOKEN_H

#include <type_traits>
#include <string>
#include <utility>
#include <variant>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include "lib/utils.h"

#undef EOF

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
  FUN, 
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
  EOF
};

struct Token {
  using typeLiteral = std::variant<std::monostate, std::string, double>;
  friend std::ostream& operator<<(std::ostream& os, const Token& token);
  Token(TokenType type, std::string lexeme, typeLiteral literal, int line) 
    : type(type), lexeme(std::move(lexeme)), literal(literal), line(line) {}
 private:
  TokenType type;
  std::string lexeme;
  typeLiteral literal;
  int line;
};

#endif
