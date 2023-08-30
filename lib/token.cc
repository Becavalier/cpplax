#include <cassert>
#include "lib/token.h"
#include "lib/helper.h"

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os 
    << std::setw(2) 
    << +enumAsInteger(token.type) 
    << " " 
    << token.lexeme 
    << " ";
  std::cout << stringifyVariantValue(token.literal);
  return os;
}
