#ifndef	_HELPER_H
#define	_HELPER_H

#include <type_traits>
#include <variant>
#include <string>
#include <sstream>
#include <cmath>
#include <limits>
#include <memory>
#include <ios>
#include <iomanip>

template<typename T> struct is_variant : std::false_type {};
template<typename ...Args> struct is_variant<std::variant<Args...>> : std::true_type {};
template<typename T> inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename Enumeration>
auto enumAsInteger(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template<typename T>
std::string stringifyVariantValue(const T& literal) {
  static_assert(is_variant_v<T>);
  if (literal.valueless_by_exception()) return "nil";
  return std::visit([](auto&& arg) {
    using K = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<K, double>) {
      std::ostringstream oss;
      double fraction, integer;
      fraction = modf(arg , &integer);
      oss << std::fixed << std::setprecision(fraction == 0 ? 0 : 9) << arg;
      const std::string& str = oss.str();
      return str.substr(0, str.find_last_not_of('0') + 1);
    } else if constexpr (std::is_same_v<K, bool>) {
      return std::string { arg ? "true" : "false" };
    } else if constexpr (std::is_same_v<K, std::string_view>) {
      return std::string { arg };
    } else if constexpr (std::is_same_v<K, std::monostate>) {
      return std::string { "nil" };
    } else {
      return std::string { "<rt-value>" };
    }
  }, literal);
}

std::string unescapeStr(const std::string&);
inline bool isDoubleEqual(double x, double y) {
  const auto epsilon = std::numeric_limits<double>::epsilon();
  return std::abs(x - y) <= epsilon * std::abs(x);
}

#endif
