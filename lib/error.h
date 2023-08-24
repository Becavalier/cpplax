#ifndef	_ERROR_H
#define	_ERROR_H

#include <string>
#include <iostream>

struct Error {
  static bool hadError;
  static void report(int line, std::string where, std::string msg) {
    std::cout << "[Line " << line << "] Error" << where << ": " << msg;
  }
  static void error(int line, std::string msg) {
    report(line, "", msg);
  }
};

bool Error::hadError = false;

#endif
