#ifndef	_HELPER_H
#define	_HELPER_H

#include <type_traits>
#include <variant>
#include <string>
#include <sstream>
#include <limits>
#include <memory>
#include <ios>
#include <iomanip>
#include <cstdint>
#include "./type.h"
#include "./object.h"

template<typename T> struct isVariant : std::false_type {};
template<typename ...Args> struct isVariant<std::variant<Args...>> : std::true_type {};
template<typename T> inline constexpr bool isVariantV = isVariant<T>::value;
template <typename Enumeration>
auto enumAsInteger(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

inline bool isDoubleEqual(double x, double y) {
  const auto epsilon = std::numeric_limits<double>::epsilon();
  return std::abs(x - y) <= epsilon * std::abs(x);
}

template<typename T>
std::string stringifyVariantValue(const T& literal) {
  static_assert(isVariantV<T>);
  if (literal.valueless_by_exception()) return "nil";
  return std::visit([&](auto&& arg) {
    using K = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<K, double>) {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(9) << arg;
      const std::string& str = oss.str();
      const std::string& strWithoutZero = oss.str().substr(0, str.find_last_not_of('0') + 1);
      return strWithoutZero.back() == '.' ? str.substr(0, strWithoutZero.size() - 1) : strWithoutZero;
    } else if constexpr (std::is_same_v<K, bool>) {
      return std::string { arg ? "true" : "false" };
    } else if constexpr (std::is_same_v<K, std::string_view>) {
      return std::string { arg };
    } else if constexpr (std::is_same_v<K, std::monostate>) {
      return std::string { "nil" };
    } else if constexpr (std::is_same_v<K, std::string>) {
      return arg;
    } else if constexpr (std::is_same_v<K, Obj*>) {
      return Obj::printObjNameByEnum(arg);
    } else if constexpr (std::is_same_v<K, std::shared_ptr<Invokable>>) {
      return std::string { arg->toString() };
    } else {
      return std::string { "<rt-value>" };
    }
  }, literal);
}

inline void printValue(const typeRuntimeValue& v) {
  std::cout << stringifyVariantValue(v);
}

std::string unescapeStr(const std::string&);

#endif
