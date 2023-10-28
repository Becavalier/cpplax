#ifndef	_HELPER_H
#define	_HELPER_H

#include <type_traits>
#include <variant>
#include <string>
#include <cstring>
#include <cmath>
#include <sstream>
#include <limits>
#include <memory>
#include <ios>
#include <iomanip>
#include <cstdint>
#include "./object.h"
#include "./type.h"

template<typename T> struct isVariant : std::false_type {};
template<typename ...Ts> struct isVariant<std::variant<Ts...>> : std::true_type {};
template<typename T> inline constexpr bool isVariantV = isVariant<T>::value;
template <typename Enumeration>
auto enumAsInteger(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template<typename T>
std::string stringifyVariantValue(const T& literal) {
  static_assert(isVariantV<T>);
  if (literal.valueless_by_exception()) return "nil";
  return std::visit([&](auto&& arg) {
    using K = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<K, typeRuntimeNumericValue>) {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(9) << arg;
      const auto& str = oss.str();
      const auto& strWithoutZero = oss.str().substr(0, str.find_last_not_of('0') + 1);
      return strWithoutZero.back() == '.' ? str.substr(0, strWithoutZero.size() - 1) : strWithoutZero;
    } else if constexpr (std::is_same_v<K, bool>) {
      return std::string { arg ? "true" : "false" };
    } else if constexpr (std::is_same_v<K, std::monostate>) {
      return std::string { "nil" };
    } else if constexpr (std::is_same_v<K, std::string>) {
      return arg;
    } else if constexpr (std::is_same_v<K, std::string_view>) {
      return std::string { arg };
    } else if constexpr (std::is_same_v<K, Obj*>) {
      return arg->toString();
    } else if constexpr (std::is_same_v<K, std::shared_ptr<Invokable>>) {
      return arg->toString();
    } else if constexpr (std::is_same_v<K, std::shared_ptr<ClassInstance>>) {
      return arg->toString();
    } else {
      return "<rt-value>"; 
    }
  }, literal);
}

inline auto isStringValue(const typeRuntimeValue& rt) {
  return std::holds_alternative<std::string>(rt) || std::holds_alternative<std::string_view>(rt);
}

inline auto isNumericValue(const typeRuntimeValue& rt) {
  return std::holds_alternative<typeRuntimeNumericValue>(rt);
}

template<typename ...Ts>
inline std::optional<std::string> stringifyValuesOfSpecifiedTypes(const typeRuntimeValue& valA, const typeRuntimeValue& valB) {
  if ((std::holds_alternative<Ts>(valA) || ...) && (std::holds_alternative<Ts>(valB) || ...)) 
    return stringifyVariantValue(valA) + stringifyVariantValue(valB);
  return std::nullopt;
} 

inline void printValue(const typeRuntimeValue& v) {
  std::cout << stringifyVariantValue(v);
}

bool isObjStringValue(const typeRuntimeValue&);
bool isDoubleEqual(const double, const double);
std::string unescapeStr(const std::string&);

#endif
