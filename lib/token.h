#ifndef _TOKEN_H
#define _TOKEN_H

#include <type_traits>
#include <string_view>
#include <utility>
#include <variant>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include "./helper.h"
#include "./type.h"

struct Token {
  friend std::ostream &operator<<(std::ostream &os, const Token &token);
  using typeLiteral = typeRuntimeValue;
  TokenType type;
  const std::string_view lexeme;
  const typeLiteral literal;   // Tokens aren’t entirely homogeneous either.
  size_t line;
  Token(const TokenType& type, const std::string_view lexeme, const typeLiteral& literal, size_t line) : type(type), lexeme(lexeme), literal(literal), line(line) {}
};

#endif
