#include <string>
#include <sstream>
#include "./helper.h"

std::string unescapeStr(const std::string& str) {
  std::ostringstream oss;
  auto curr = cbegin(str);
  while (curr != cend(str)) {
    if (*curr == '\\') {
      const auto next = curr + 1;
      if (next != cend(str)) {
        switch(*next) {
          // TODO: dealing with double '\' and other escaping characters.
          case 'n': {
            oss << '\n'; 
            curr += 2; 
            break;
          }
          case 't': {
            oss << '\t'; 
            curr += 2; 
            break;
          }
          case 'r': {
            oss << '\r'; 
            curr += 2; 
            break;
          }
          default: oss << *curr++;
        }
      } else {
        oss << *curr++;
      }
    } else {
      oss << *curr++;
    }
  }
  return oss.str();
}
