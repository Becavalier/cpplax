#ifndef	_ERROR_H
#define	_ERROR_H

#include <string>
#include <iostream>
#include <utility>

struct Error {
  static bool hadError;
  static void report(
    int line, 
    const std::string&& where, 
    const std::string&& msg) {
    std::cout 
      << "[Line " << line << "] Error " 
      << where << ": " << msg 
      << std::endl;
    hadError = true;
  }
  static void error(int line, std::string&& msg) {
    report(line, "", std::move(msg));
  }
};

#endif
