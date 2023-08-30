#ifndef	_HELPER_H
#define	_HELPER_H

#include <type_traits>
#include <variant>
#include <sstream>

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
  if (literal.valueless_by_exception()) return "";
  return std::visit([](auto&& arg) {
    using K = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<K, double>) {
      std::ostringstream oss;
      oss << std::noshowpoint << arg;
      return oss.str();
    } else if constexpr (std::is_same_v<K, bool>) {
      return std::string { arg ? "true" : "false" };
    } else if constexpr (std::is_same_v<K, std::string>) {
      return arg;
    } else {
      return std::string { "nil" };
    }
  }, literal);
}

#endif
