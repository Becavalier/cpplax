#ifndef	_ERROR_H
#define	_ERROR_H

#include <string>
#include <string_view>
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
    size_t line, 
    const std::string_view where, 
    const std::string_view msg) {
    std::cerr << "[Line " << line << "] Error: ";
    if (!where.empty()) std::cerr << "at \"" << where << "\"" << ", ";
    std::cerr << msg << std::endl;
    hadError = true;
  }
  static void error(const Token& token, const std::string_view msg) {
    report(token.line, token.type == TokenType::SOURCE_EOF ? "end" : "'" + std::string { token.lexeme } + "'", msg);
  }
  static void error(size_t line, const std::string_view msg) {
    report(line, "", msg);
  }
  static void error(size_t line, std::string_view where, const std::string_view msg) {
    report(line, where, msg);
  }
  static void runtimeError(const RuntimeError& err) {
    error(err.token, err.what());
    hadRuntimeError = true;
  }
};

#endif
