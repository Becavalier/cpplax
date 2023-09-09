#ifndef	_ERROR_H
#define	_ERROR_H

#include <string>
#include <iostream>
#include <utility>
#include "./token.h"

struct RuntimeError : public std::exception {
  const Token& token;
  const std::string msg;
 public:
  RuntimeError(const Token& token, const std::string& msg) : token(token), msg(msg) {}
  const char* what(void) const noexcept {
    return msg.data();
  }
};

struct Error {
  static bool hadError;
  static bool hadRuntimeError;
  static void report(
    int line, 
    const std::string& where, 
    const std::string& msg) {
    std::cerr << "[Line " << line << "] Error";
    if (!where.empty()) std::cerr << ": " << where;
    std::cerr << ": " << msg << std::endl;
    hadError = true;
  }
  static void error(const Token& token, const std::string& msg) {
    report(token.line, token.type == TokenType::SOURCE_EOF ? "at end" : "at '" + token.lexeme + "'", msg);
  }
  static void error(int line, const std::string& msg) {
    report(line, "", msg);
  }
  static void runtimeError(const RuntimeError& error) {
    report(error.token.line, "", error.what());
    hadRuntimeError = true;
  }
};

#endif
