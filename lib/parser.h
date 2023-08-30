#ifndef	_PARSER_H
#define	_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "lib/token.h"
#include "lib/expr.h"
#include "lib/error.h"

class ParseError : public std::exception {};

class Parser {
  const std::vector<Token>& tokens;
  std::vector<Token>::const_iterator current;
  
  static auto error(const Token& token, const std::string& msg) {
    Error::error(token, msg);
    return ParseError {};
  }
  auto& peek(void) {
    return *current;
  }
  auto& previous(void) {
    return *(current - 1);
  }
  auto isAtEnd(void) {
    return current == tokens.cend();
  }
  auto& advance(void) {
    if (!isAtEnd()) current++;
    return previous();
  }
  auto check(const TokenType& type) {
    return isAtEnd() ? false : peek().type == type;
  }
  void synchronize(void) {
    advance();
    while (!isAtEnd()) {
      if (previous().type == TokenType::SEMICOLON) return;
      switch (peek().type) {  // Check if it's the statement boundary.
        case TokenType::CLASS:
        case TokenType::FUN:
        case TokenType::VAR:
        case TokenType::FOR:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::PRINT:
        case TokenType::RETURN:
          return;
        default:;
      }
      advance();
    }
  }
  auto match(const std::vector<TokenType>& types) {
    for (const auto& type : types) {  // Check if it matches either of the rules.
      if (check(type)) {
        advance();
        return true;
      }
    }
    return false;
  }
  auto match(const TokenType& type) {
    if (check(type)) {
      advance();
      return true;
    }
    return false;
  }
  const auto& consume(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    throw error(peek(), msg);
  }
  std::shared_ptr<Expr> expression(void) {
    // expression → equality ;
    return equality();
  }
  std::shared_ptr<Expr> equality(void) {
    // equality → comparison ( ( "!=" | "==" ) comparison )* ;
    auto expr = comparison();
    while (match({ TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL, })) {
      const auto& op = previous();
      auto right = comparison();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<Expr> comparison(void) {
    // comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    auto expr = term();
    while (match({ TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL, })) {
      const auto& op = previous();
      auto right = term();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<Expr> term(void) {
    // term → factor ( ( "-" | "+" ) factor )* ;
    auto expr = factor();
    while (match({ TokenType::MINUS, TokenType::PLUS, })) {
      const auto& op = previous();
      auto right = factor();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<Expr> factor(void) {
    // factor → unary ( ( "/" | "*" ) unary )* ;
    auto expr = unary();
    while (match({ TokenType::SLASH, TokenType::STAR, })) {
      const auto& op = previous();
      auto right = unary();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<Expr> unary(void) {
    // unary → ( "!" | "-" ) unary | primary ;
    if (match({ TokenType::BANG, TokenType::MINUS, })) {
      const auto& op = previous();
      auto right = unary();
      return std::make_shared<UnaryExpr>(op, right);
    }
    return primary();
  }
  std::shared_ptr<Expr> primary(void) {
    // primary → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
    if (match(TokenType::FALSE)) return std::make_shared<LiteralExpr>(false);
    if (match(TokenType::TRUE)) return std::make_shared<LiteralExpr>(true);
    if (match(TokenType::NIL)) return std::make_shared<LiteralExpr>(std::monostate {});
    if (match({ TokenType::NUMBER, TokenType::STRING, })) {
      return std::make_shared<LiteralExpr>(previous().literal);
    }
    if (match(TokenType::LEFT_PAREN)) {
      auto expr = expression();
      consume(TokenType::RIGHT_PAREN, "expect ')' after expression.");  // Find the closing pair.
      return std::make_shared<GroupingExpr>(expr);
    }
    throw error(peek(), "expect expression.");
  }
 public:
  Parser(const std::vector<Token>& tokens) : tokens(tokens) {
    current = tokens.cbegin();
  }
  std::shared_ptr<Expr> parse(void) {
    try {
      return expression();
    } catch(ParseError error) {
      return nullptr;
    }
  }
};

#endif
