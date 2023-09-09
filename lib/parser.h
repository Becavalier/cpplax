#ifndef	_PARSER_H
#define	_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "./token.h"
#include "./expr.h"
#include "./stmt.h"
#include "./error.h"
#include "./helper.h"

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
    if (!isAtEnd()) ++current;
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
        case TokenType::FN:
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
  Stmt::sharedStmtPtr varDeclaration(void) {
    // varDecl → "var" IDENTIFIER ( "=" expression )? ";" ;
    const Token& name = consume(TokenType::IDENTIFIER, "expect variable name.");
    Expr::sharedExprPtr initializer = nullptr;
    if (match(TokenType::EQUAL)) {
      initializer = expression();
    }
    consume(TokenType::SEMICOLON, "expect ';' after variable declaration.");
    return std::make_shared<VarStmt>(name, initializer);
  }
  std::shared_ptr<FunctionStmt> function(const std::string& kind) {
    const auto& name = consume(TokenType::IDENTIFIER, "expect " + kind + " name.");
    consume(TokenType::LEFT_PAREN, "expect '(' after " + kind + " name.");
    std::vector<std::reference_wrapper<const Token>> parameters;
    if (!check(TokenType::RIGHT_PAREN)) {
      do {
        if (parameters.size() >= 255) {
          error(peek(), "can't have more than 255 parameters.");
        }
        parameters.push_back(consume(TokenType::IDENTIFIER, "expect parameter name."));
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "expect '{' before " + kind + " body.");
    std::vector<Stmt::sharedStmtPtr> body = block();
    return std::make_shared<FunctionStmt>(name, parameters, body);
  }
  Stmt::sharedStmtPtr declaration(void) {
    try {
      if (match(TokenType::FN)) return function("function");
      if (match(TokenType::VAR)) return varDeclaration();
      return statement();
    } catch (const ParseError& parserError) {
      synchronize();
      return nullptr;
    }
  }
  auto ifStatement(void) {
    consume(TokenType::LEFT_PAREN, "expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RIGHT_PAREN, "expect ')' after if condition.");
    auto thenBranch = statement();   // Either one single statement or a block statement.
    Stmt::sharedStmtPtr elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
      elseBranch = statement();
    }
    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
  }
  auto whileStatement(void) {
    consume(TokenType::LEFT_PAREN, "expect '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RIGHT_PAREN, "expect ')' after while condition.");
    auto body = statement(); 
    return std::make_shared<WhileStmt>(condition, body);
  }
  auto forStatement(void) {
    // forStmt → "for" "(" ( varDecl | exprStmt | ";" )
    //           expression? ";"
    //           expression? ")" statement ;
    // Desugar to "while" loop.
    consume(TokenType::LEFT_PAREN, "expect '(' after 'for'.");
    Stmt::sharedStmtPtr initializer;
    if (match(TokenType::SEMICOLON)) {
      initializer = nullptr;
    } else if (match(TokenType::VAR)) {
      initializer = varDeclaration();
    } else {
      initializer = expressionStatement();
    }
    Expr::sharedExprPtr condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
      condition = expression();
    }
    consume(TokenType::SEMICOLON, "expect ';' after loop condition.");
    Expr::sharedExprPtr increment = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
      increment = expression();
    }
    consume(TokenType::RIGHT_PAREN, "expect ')' after for clauses.");
    Stmt::sharedStmtPtr body = statement();
    if (increment != nullptr) {
      body = std::make_shared<BlockStmt>(
        std::vector<Stmt::sharedStmtPtr> { 
          body, std::make_shared<ExpressionStmt>(increment),  // Append the increment to the iteration body.
        }
      );
    }
    if (condition == nullptr) condition = std::make_shared<LiteralExpr>(true);  // Dead loop if no condition set.
    body = std::make_shared<WhileStmt>(condition, body);
    if (initializer != nullptr) {
      body = std::make_shared<BlockStmt>(
        std::vector<Stmt::sharedStmtPtr> { 
          initializer, body,  // Prepend the initializer above the while loop.
        }
      );
    }
    return body;
  }
  auto printStatement(void) {
    // printStmt → "print" expression ";" ;
    auto value = expression();
    consume(TokenType::SEMICOLON, "expect ';' after value.");
    return std::make_shared<PrintStmt>(value);
  }
  auto returnStatement(void) {
    // returnStmt → "return" expression? ";" ;
    auto keyword = previous();
    Expr::sharedExprPtr value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
      value = expression();
    }
    consume(TokenType::SEMICOLON, "expect ';' after return value.");
    return std::make_shared<ReturnStmt>(keyword, value);
  }
  Stmt::sharedStmtPtr expressionStatement(void) {
    // exprStmt → expression ";" ;
    auto expr = expression();
    consume(TokenType::SEMICOLON, "expect ';' after expression.");
    return std::make_shared<ExpressionStmt>(expr);
  }
  std::vector<Stmt::sharedStmtPtr> block(void) {
    // block → "{" declaration* "}" ;
    std::vector<Stmt::sharedStmtPtr> declarations;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
      declarations.push_back(declaration());
    }
    consume(TokenType::RIGHT_BRACE, "expect '}' after block.");
    return declarations;
  }
  Stmt::sharedStmtPtr statement(void) {
    // statement → exprStmt | printStmt ;
    if (match(TokenType::FOR)) return forStatement();
    if (match(TokenType::IF)) return ifStatement();
    if (match(TokenType::PRINT)) return printStatement();
    if (match(TokenType::RETURN)) return returnStatement();
    if (match(TokenType::WHILE)) return whileStatement();
    if (match(TokenType::LEFT_BRACE)) return std::make_shared<BlockStmt>(block());
    /**
     * If the next token doesn’t look like any known kind of statement, -
     * we assume it must be an expression statement. That’s the typical -
     * final fallthrough case when parsing a statement, since it’s hard -
     * to proactively recognize an expression from its first token.
    */
    return expressionStatement();
  }
  Expr::sharedExprPtr logicalOr(void) {
    // Lower precedence.
    // logic_or → logic_and ( "or" logic_and )* ;
    auto expr = logicalAnd();
    while (match(TokenType::OR)) {
      const auto& op = previous();
      auto right = logicalAnd();
      expr = std::make_shared<LogicalExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr logicalAnd(void) {
    // With higher precedence.
    // logic_and → equality ( "and" equality )* ;
    auto expr = equality();
    while (match(TokenType::AND)) {
      const auto& op = previous();
      auto right = equality();
      expr = std::make_shared<LogicalExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr finishCall(Expr::sharedExprPtr callee) {
    // Pasre the callee with its arguments.
    // arguments → expression ( "," expression )* ;
    std::vector<Expr::sharedExprPtr> arguments;
    if (!check(TokenType::RIGHT_PAREN)) {
      do {
        if (arguments.size() >= 255) {
          error(peek(), "can't have more than 255 arguments.");
        }
        arguments.push_back(expression());
      } while (match(TokenType::COMMA));
    }
    auto paren = consume(TokenType::RIGHT_PAREN, "expect ')' after arguments.");
    return std::make_shared<CallExpr>(callee, paren, arguments);
  }
  Expr::sharedExprPtr assignment(void) {
    // assignment → IDENTIFIER "=" assignment 
    //            | logic_or ;
    /**
     * (AssignExpr varA 
     *   (AssignExpr varB 
     *     (AssignExpr varC (equality))
     *   )
     * )
     * 
    */
    auto expr = logicalOr();
    if (match(TokenType::EQUAL)) {
      const auto& equals = previous();
      auto value = assignment();
      // Only certain types of expression could be lvalues.
      if (auto castPtr = std::dynamic_pointer_cast<VariableExpr>(expr)) {
        const auto& name = castPtr->name;
        return std::make_shared<AssignExpr>(name, value);
      }
      error(equals, "invalid assignment target.");
    }
    return expr;
  }
  Expr::sharedExprPtr expression(void) {
    // expression → assignment ;
    return assignment();
  }
  Expr::sharedExprPtr equality(void) {
    // equality → comparison ( ( "!=" | "==" ) comparison )* ;
    auto expr = comparison();
    while (match({ TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL, })) {
      const auto& op = previous();
      auto right = comparison();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr comparison(void) {
    // comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    auto expr = term();
    while (match({ TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL, })) {
      const auto& op = previous();
      auto right = term();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr term(void) {
    // term → factor ( ( "-" | "+" ) factor )* ;
    auto expr = factor();
    while (match({ TokenType::MINUS, TokenType::PLUS, })) {
      const auto& op = previous();
      auto right = factor();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr factor(void) {
    // factor → unary ( ( "/" | "*" ) unary )* ;
    auto expr = unary();
    while (match({ TokenType::SLASH, TokenType::STAR, })) {
      const auto& op = previous();
      auto right = unary();
      expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
  }
  Expr::sharedExprPtr unary(void) {
    // unary → ( "!" | "-" ) unary | call ;
    if (match({ TokenType::BANG, TokenType::MINUS, })) {
      const auto& op = previous();
      auto right = unary();
      return std::make_shared<UnaryExpr>(op, right);
    }
    return call();
  }
  Expr::sharedExprPtr call(void) {
    // call → primary ( "(" arguments? ")" )* ;
    auto expr = primary();
    while (true) {
      if (match(TokenType::LEFT_PAREN)) {
        expr = finishCall(expr);
      } else {
        break;
      }
    }
    return expr;
  }
  Expr::sharedExprPtr primary(void) {
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
  explicit Parser(const std::vector<Token>& tokens) : current(tokens.cbegin()) {}
  auto parse(void) {
    // program → declaration* EOF ;
    std::vector<Stmt::sharedStmtPtr> statements;
    while (!isAtEnd()) {
      statements.push_back(declaration());
    }
    return statements;
  }
};

#endif
