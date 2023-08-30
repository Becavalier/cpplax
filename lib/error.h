#ifndef	_ERROR_H
#define	_ERROR_H

#include <string>
#include <iostream>
#include <utility>
#include "lib/token.h"

struct RuntimeError : public std::exception {
  const Token& token;
  const char* msg;
 public:
  RuntimeError(const Token& token, const char* msg) : token(token), msg(msg) {}
  const char* what(void) {
    return msg;
  }
};

struct Error {
  static bool hadError;
  static bool hadRuntimeError;
  static void report(
    int line, 
    const std::string& where, 
    const std::string& msg) {
    std::cout 
      << "[Line " << line << "] Error " 
      << where << ": " << msg 
      << std::endl;
    hadError = true;
  }
  static void error(const Token& token, const std::string& msg) {
    report(token.line, token.type == TokenType::EOF ? "at end" : "at '" + token.lexeme + "'", msg);
  }
  static void error(int line, const std::string& msg) {
    report(line, "", msg);
  }
  static void runtimeError(RuntimeError& error) {
    std::cerr << error.what() << "\n[line " << error.token.line << ']';
    hadRuntimeError = true;
  }
};

#endif
