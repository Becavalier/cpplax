#ifndef	_PARSER_H
#define	_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "lib/token.h"
#include "lib/expr.h"
#include "lib/stmt.h"
#include "lib/error.h"
#include "lib/helper.h"

class ParseError : public std::exception {};

class Parser {
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
    return peek().type == TokenType::SOURCE_EOF;  // Stop when meeting EOF.
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
  std::shared_ptr<Stmt> varDeclaration(void) {
    // varDecl → "var" IDENTIFIER ( "=" expression )? ";" ;
    const Token& name = consume(TokenType::IDENTIFIER, "expect variable name.");
    std::shared_ptr<Expr> initializer = nullptr;
    if (match(TokenType::EQUAL)) {
      initializer = expression();
    }
    consume(TokenType::SEMICOLON, "expect ';' after variable declaration.");
    return std::make_shared<VarStmt>(name, initializer);
  }
  std::shared_ptr<Stmt> declaration(void) {
    try {
      if (match(TokenType::VAR)) return varDeclaration();
      return statement();
    } catch (ParseError error) {
      synchronize();
      return nullptr;
    }
  }
  auto printStatement(void) {
    // printStmt → "print" expression ";" ;
    auto value = expression();
    consume(TokenType::SEMICOLON, "expect ';' after value.");
    return std::make_shared<PrintStmt>(value);
  }
  std::shared_ptr<Stmt> expressionStatement(void) {
    // exprStmt → expression ";" ;
    auto expr = expression();
    consume(TokenType::SEMICOLON, "expect ';' after expression.");
    return std::make_shared<ExpressionStmt>(expr);
  }
  auto block(void) {
    // block → "{" declaration* "}" ;
    std::vector<std::shared_ptr<Stmt>> declarations;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
      declarations.push_back(declaration());
    }
    consume(TokenType::RIGHT_BRACE, "expect '}' after block.");
    return declarations;
  }
  std::shared_ptr<Stmt> statement(void) {
    // statement → exprStmt | printStmt ;
    if (match(TokenType::PRINT)) return printStatement();
    if (match(TokenType::LEFT_BRACE)) return std::make_shared<BlockStmt>(block());
    /**
     * If the next token doesn’t look like any known kind of statement, -
     * we assume it must be an expression statement. That’s the typical -
     * final fallthrough case when parsing a statement, since it’s hard -
     * to proactively recognize an expression from its first token.
    */
    return expressionStatement();
  }
  std::shared_ptr<Expr> assignment(void) {
    // assignment → IDENTIFIER "=" assignment 
    //            | equality ;
    /**
     * (AssignExpr varA 
     *   (AssignExpr varB 
     *     (AssignExpr varC (equality))
     *   )
     * )
     * 
    */
    auto expr = equality();  // Expr only.
    if (match(TokenType::EQUAL)) {
      const auto& equals = previous();
      auto value = assignment();
      // Only certain types of expression could be lvalue.
      if (instanceof<VariableExpr>(expr.get())) {  
        const auto& name = std::dynamic_pointer_cast<VariableExpr>(expr)->name;
        return std::make_shared<AssignExpr>(name, value);
      }
      error(equals, "invalid assignment target.");
    }
    return expr;
  }
  std::shared_ptr<Expr> expression(void) {
    // expression → assignment ;
    return assignment();
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
    if (match(TokenType::IDENTIFIER)) {
      return std::make_shared<VariableExpr>(previous());
    }
    if (match(TokenType::LEFT_PAREN)) {
      auto expr = expression();
      consume(TokenType::RIGHT_PAREN, "expect ')' after expression.");  // Find the closing pair.
      return std::make_shared<GroupingExpr>(expr);
    }
    throw error(peek(), "expect expression.");
  }
 public:
  Parser(const std::vector<Token>& tokens) {
    current = tokens.cbegin();
  }
  auto parse(void) {
    // program → declaration* EOF ;
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!isAtEnd()) {
      statements.push_back(declaration());
    }
    return statements;
  }
};

#endif
