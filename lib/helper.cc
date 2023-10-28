#include <string>
#include <sstream>
#include "./helper.h"
 
bool isObjStringValue(const typeRuntimeValue& rt) {
  return std::holds_alternative<Obj*>(rt) && std::get<Obj*>(rt)->type == ObjType::OBJ_STRING;
}

/**
 * There can be subtle rounding differences between platforms, -
 * so allow for some error. For example, GCC uses 80 bit registers for doubles on non-Darwin x86 platforms. 
 * A change in when values are written to memory (when it is shortened to 64 bits), -
 * or comparing values between Darwin and non-Darwin platforms could lead to different results.
*/
bool isDoubleEqual(const double a, const double b) {
  // Bitwise comparison to avoid issues with infinity and NaN.
  if (std::memcmp(&a, &b, sizeof(double)) == 0) return true;
  // If the sign bit is different, then it is definitely not equal. This handles cases like -0 != 0.
  if (std::signbit(a) != std::signbit(b)) return false;
  const double ep = (std::abs(a) + std::abs(b)) * std::numeric_limits<double>::epsilon();
  return std::abs(a - b) <= ep;
}

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
