#include <cassert>
#include "lib/token.h"

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os 
    << std::setw(2) 
    << +enumAsInteger(token.type) 
    << " " 
    << token.lexeme 
    << " ";
  std::cout << Token::stringifyLiteralValue(token.literal);
  return os;
}

std::string Token::stringifyLiteralValue(const typeLiteral& literal) {
  assert(literal.valueless_by_exception() == false);
  return std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, double>) {
      return std::to_string(arg);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return arg;
    } else {
      return std::string { "nil" };
    }
  }, literal);
} 
