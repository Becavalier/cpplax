#ifndef	_UTILS_H
#define	_UTILS_H

#include <type_traits>

template <typename Enumeration>
auto enumAsInteger(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

#endif
