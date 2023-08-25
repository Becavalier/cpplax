#include "lib/token.h"

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os 
    << std::setw(2) 
    << +enumAsInteger(token.type) 
    << " " 
    << token.lexeme 
    << " ";
  std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, std::string>) {
      std::cout << arg;
    }
  }, token.literal);
  return os;
}
