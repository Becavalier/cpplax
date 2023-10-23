#ifndef	_NATIVE_H
#define	_NATIVE_H

#include <cstdint>
#include <iostream>
#include <ctime>
#include "./type.h"
#include "./helper.h"

typeRuntimeValue nativePrint(uint8_t argCount, typeVMStack::const_iterator args) {
  for (auto i = 0; i < argCount; i++) {
    std::cout << stringifyVariantValue(*(args + i));
  }
  return std::monostate {};
}

typeRuntimeValue nativeClock(uint8_t, typeVMStack::const_iterator) {
  return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}

#endif
