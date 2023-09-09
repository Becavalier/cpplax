#ifndef _TOKEN_H
#define _TOKEN_H

#include <type_traits>
#include <string>
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
  std::string lexeme;
  typeLiteral literal;
  int line;
  Token(const TokenType& type, const std::string& lexeme, const typeLiteral& literal, int line) 
    : type(type), lexeme(lexeme), literal(literal), line(line) {}
};

#endif
