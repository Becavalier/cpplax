#ifndef	_COMPILER_H
#define	_COMPILER_H

#define DEBUG_PRINT_CODE

#include <string>
#include <cstdlib>
#include <array>
#include <cstdint>
#include "./chunk.h"
#include "./token.h"
#include "./error.h"

struct Compiler;
// using ParseFn = void (Compiler::*)();
typedef void (Compiler::*parseFn)();

enum Precedence : uint8_t {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
};

struct ParseRule {
  parseFn prefix;
  parseFn infix;
  Precedence precedence;
};

struct Compiler {
  bool panicMode = false;
  const std::vector<Token>& tokens;
  std::vector<Token>::const_iterator current;
  Chunk chunk;
  const ParseRule rules[TokenType::TOTAL] = {
    [TokenType::LEFT_PAREN] = { &Compiler::grouping, nullptr, Precedence::PREC_NONE },
    [TokenType::RIGHT_PAREN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::LEFT_BRACE] = { nullptr, nullptr, Precedence::PREC_NONE }, 
    [TokenType::RIGHT_BRACE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::COMMA] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::DOT] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::MINUS] = { &Compiler::unary, &Compiler::binary, Precedence::PREC_TERM },
    [TokenType::PLUS] = {nullptr, &Compiler::binary, Precedence::PREC_TERM },
    [TokenType::SEMICOLON] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SLASH] = {nullptr, &Compiler::binary, Precedence::PREC_FACTOR },
    [TokenType::STAR] = {nullptr, &Compiler::binary, Precedence::PREC_FACTOR },
    [TokenType::BANG] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::BANG_EQUAL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::EQUAL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::EQUAL_EQUAL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::GREATER] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::GREATER_EQUAL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::LESS] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::LESS_EQUAL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::IDENTIFIER] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::STRING] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::NUMBER] = {&Compiler::number, nullptr, Precedence::PREC_NONE },
    [TokenType::AND] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::CLASS] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::ELSE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FALSE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FOR] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FN] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::IF] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::NIL] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::OR] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::RETURN] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SUPER] = {nullptr, nullptr, Precedence::PREC_NONE } ,
    [TokenType::THIS] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::TRUE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::VAR] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::WHILE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::ERROR] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SOURCE_EOF] = {nullptr, nullptr, Precedence::PREC_NONE },
  };
  explicit Compiler(const std::vector<Token>& tokens) : tokens(tokens), current(tokens.cbegin()) {}
  void consume(TokenType type, const char* msg) {
    if ((*current).type == type) {
      ++current;
      return;
    }
    errorAtCurrent(msg);
  }
  auto& peek(void) {
    return *current;
  }
  auto previous(void) {
    return *(current - 1);
  }
  void errorAt(const Token& token, const char* msg) {
    if (panicMode) return;
    panicMode = true;
    Error::error(token, msg);  // Error::hadError -> true.
  }
  void errorAtCurrent(const char* msg) {
    errorAt(*current, msg);
  }
  void error(const char* msg) {
    errorAt(previous(), msg);
  }
  void emitByte(OpCodeType byte) {
    chunk.addCode(byte, previous().line);
  }
  void emitBytes(OpCodeType byteX, OpCodeType byteY) {
    emitByte(byteX);
    emitByte(byteY);
  }
  void emitConstant(typeRTNumericValue value) {
    const auto constantIdx = chunk.addConstant(value);
    if (constantIdx <= UINT8_MAX) {
      emitBytes(OpCode::OP_CONSTANT, static_cast<OpCodeType>(constantIdx));
    } else {
      error("too many constants in one chunk.");
    }
  }
  void parsePrecedence(Precedence precedence) {
    ++current;
    auto prefixRule = getRule(previous().type)->prefix;
    if (prefixRule == nullptr) {
      error("expect expression.");
      return;
    }
    (this->*prefixRule)();
    while (precedence <= getRule(peek().type)->precedence) {
      ++current;
      auto infixRule = getRule(previous().type)->infix;
      (this->*infixRule)();
    }
  }
  const ParseRule* getRule(TokenType type) {
    return &rules[type];
  }
  void number(void) {
    emitConstant(std::get<typeRTNumericValue>(previous().literal));
  }
  void grouping(void) {
    expression();
    consume(TokenType::RIGHT_PAREN, "expect ')' after expression.");
  }
  void unary() {
    auto opType = previous().type;
    parsePrecedence(Precedence::PREC_UNARY);  // Compile the operand.
    switch (opType) {
      case TokenType::MINUS: emitByte(OpCode::OP_NEGATE); break;
      default: return;  // Unreachable.
    }
  }
  void binary(void) {
    auto opType = previous().type;
    auto rule = getRule(opType);
    parsePrecedence(static_cast<Precedence>(rule->precedence + 1));
    switch (opType) {
      case TokenType::PLUS: emitByte(OpCode::OP_ADD); break;
      case TokenType::MINUS: emitByte(OpCode::OP_SUBTRACT); break;
      case TokenType::STAR: emitByte(OpCode::OP_MULTIPLY); break;
      case TokenType::SLASH: emitByte(OpCode::OP_DIVIDE); break;
      default: return;
    }
  }
  void expression(void) {
    parsePrecedence(Precedence::PREC_ASSIGNMENT);
  }
  Chunk compile(void) {
    expression();
    consume(TokenType::SOURCE_EOF, "expect end of expression.");
    emitByte(OpCode::OP_RETURN);
// #ifdef DEBUG_PRINT_CODE
//     if (!Error::hadError) {
//       ChunkDebugger::disassembleChunk(chunk, "code");
//     }
// #endif   
    return chunk;
  }
};

#endif
