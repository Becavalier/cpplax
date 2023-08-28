#ifndef	_PARSER_H
#define	_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "lib/token.h"
#include "lib/expr.h"

class Parser {
  const std::vector<Token>& tokens;
  std::vector<Token>::const_iterator current;
  auto& peek(void) {
    return *current;
  }
  auto& previous(void) {
    return *(current - 1);
  }
  bool isAtEnd(void) {
    return current == tokens.cend();
  }
  auto& advance(void) {
    if (!isAtEnd()) current++;
    return previous();
  }
  bool check(TokenType type) {
    return isAtEnd() ? peek().type == type : false;
  }
  bool match(const std::vector<TokenType>& types) {
    for (const auto type : types) {  // Check if it matches either of the rules.
      if (check(type)) {
        advance();
        return true;
      }
    }
    return false;
  }
  bool match(const TokenType& type) {
    if (check(type)) {
      advance();
      return true;
    }
    return false;
  }
  Token consume(TokenType type, std::string msg) {

  }
  std::shared_ptr<const Expr> expression(void) {
    // expression → equality ;
    return equality();
  }
  std::shared_ptr<const Expr> equality(void) {
    // equality → comparison ( ( "!=" | "==" ) comparison )* ;
    auto expr = comparison();
    while (match({ TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL, })) {
      const auto& op = previous();
      auto right = comparison();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<const Expr> comparison(void) {
    // comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    auto expr = term();
    while (match({ TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL, })) {
      const auto& op = previous();
      auto right = term();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<const Expr> term(void) {
    // term → factor ( ( "-" | "+" ) factor )* ;
    auto expr = factor();
    while (match({ TokenType::MINUS, TokenType::PLUS, })) {
      const auto& op = previous();
      auto right = factor();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<const Expr> factor(void) {
    // factor → unary ( ( "/" | "*" ) unary )* ;
    auto expr = unary();
    while (match({ TokenType::SLASH, TokenType::STAR, })) {
      const auto& op = previous();
      auto right = unary();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  std::shared_ptr<const Expr> unary(void) {
    // unary → ( "!" | "-" ) unary | primary ;
    if (match({ TokenType::BANG, TokenType::MINUS, })) {
      const auto& op = previous();
      auto right = unary();
      return std::make_shared<UnaryExpr>(op, right);
    }
    return primary();
  }
  std::shared_ptr<const Expr> primary(void) {
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
  }
 public:
  Parser(const std::vector<Token>& tokens) : tokens(tokens) {
    current = tokens.cbegin();
  }
  std::shared_ptr<const Expr> parse(void) {
    return expression();
  }
};

#endif
