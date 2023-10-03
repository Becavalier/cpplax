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
using typeParseFn = void (Compiler::*)();

// All prefix operators in Lox have the same precedence.
enum Precedence : uint8_t {  // Precedence: lowest -> highest.
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
  typeParseFn prefix;
  typeParseFn infix;
  Precedence precedence;
};

struct Compiler {
  bool panicMode = false;
  std::vector<Token>& tokens;
  std::vector<Token>::const_iterator current;
  Chunk chunk;
  HeapObj* objs;  // Points to the head of the heap object list.
  /**
   * Rule table for "Pratt Parser". Columns are:
   * - The function to compile a prefix expression starting with a token of that type.
   * - The function to compile an infix expression whose left operand is followed by a token of that type.
   * - The precedence of an infix expression that uses that token as an operator.
  */
  const ParseRule rules[TokenType::TOTAL] = {
    [TokenType::LEFT_PAREN] = { &Compiler::grouping, nullptr, Precedence::PREC_NONE },
    [TokenType::RIGHT_PAREN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::LEFT_BRACE] = { nullptr, nullptr, Precedence::PREC_NONE }, 
    [TokenType::RIGHT_BRACE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::COMMA] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::DOT] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::MINUS] = { &Compiler::unary, &Compiler::binary, Precedence::PREC_TERM },
    [TokenType::PLUS] = { nullptr, &Compiler::binary, Precedence::PREC_TERM },
    [TokenType::SEMICOLON] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SLASH] = { nullptr, &Compiler::binary, Precedence::PREC_FACTOR },
    [TokenType::STAR] = { nullptr, &Compiler::binary, Precedence::PREC_FACTOR },
    [TokenType::BANG] = { &Compiler::unary, nullptr, Precedence::PREC_NONE },
    [TokenType::BANG_EQUAL] = { nullptr, &Compiler::binary, Precedence::PREC_EQUALITY },
    [TokenType::EQUAL] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::EQUAL_EQUAL] = { nullptr, &Compiler::binary, Precedence::PREC_EQUALITY },
    [TokenType::GREATER] = { nullptr, &Compiler::binary, Precedence::PREC_COMPARISON },
    [TokenType::GREATER_EQUAL] = { nullptr, &Compiler::binary, Precedence::PREC_COMPARISON },
    [TokenType::LESS] = { nullptr, &Compiler::binary, Precedence::PREC_COMPARISON },
    [TokenType::LESS_EQUAL] = { nullptr, &Compiler::binary, Precedence::PREC_COMPARISON },
    [TokenType::IDENTIFIER] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::STRING] = { &Compiler::string, nullptr, Precedence::PREC_NONE },
    [TokenType::NUMBER] = { &Compiler::number, nullptr, Precedence::PREC_NONE },
    [TokenType::AND] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::CLASS] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::ELSE] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FALSE] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::FOR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::IF] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::NIL] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::OR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::RETURN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SUPER] = { nullptr, nullptr, Precedence::PREC_NONE } ,
    [TokenType::THIS] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::TRUE] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::VAR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::WHILE] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SOURCE_EOF] = { nullptr, nullptr, Precedence::PREC_NONE },
  };
  explicit Compiler(std::vector<Token>& tokens) : tokens(tokens), current(tokens.cbegin()), objs(nullptr) {}
  auto& peek(void) {
    return *current;
  }
  void advance(void) {
    ++current;
  }
  auto previous(void) {
    return *(current - 1);
  }
  void consume(TokenType type, const char* msg) {
    if (peek().type == type) {
      advance();
      return;
    }
    errorAtCurrent(msg);
  }
  void errorAt(const Token& token, const char* msg) {
    // if (panicMode) return;
    // panicMode = true;
    Error::error(token, msg);  // Error::hadError -> true.
  }
  void errorAtCurrent(const char* msg) {
    errorAt(peek(), msg);
  }
  void errorAtPrevious(const char* msg) {
    errorAt(previous(), msg);  // Report an error at the location of the just consumed token.
  }
  void emitByte(OpCodeType byte) {
    chunk.addCode(byte, previous().line);
  }
  void emitByte(OpCodeType byte, size_t line) {
    chunk.addCode(byte, line);
  }
  void emitConstant(typeRuntimeValue value) {
    const auto constantIdx = chunk.addConstant(value);
    if (constantIdx <= UINT8_MAX) {
      emitByte(OpCode::OP_CONSTANT);
      emitByte(static_cast<OpCodeType>(constantIdx));
    } else {
      errorAtPrevious("too many constants in one chunk.");
    }
  }
  void parsePrecedence(Precedence precedence) {  // Core driving function.
    advance();
    auto prefixRule = getRule(previous().type)->prefix;
    if (prefixRule == nullptr) {
      errorAtPrevious("expect expression.");
      return;
    }
    (this->*prefixRule)();
    while (precedence <= getRule(peek().type)->precedence) {  // The prefix expression already compiled might be an operand for the infix operator.
      advance();
      auto infixRule = getRule(previous().type)->infix;
      (this->*infixRule)();
    }
  }
  const ParseRule* getRule(TokenType type) {
    return &rules[type];
  }
  void string(void) {
    emitConstant(new HeapStringObj { std::get<std::string_view>(previous().literal), &objs });  // Generate a sting object on the heap.
  }
  void number(void) {
    emitConstant(previous().literal);  // Number constant has been consumed.
  }
  void grouping(void) {
    expression();  // The opening '(' has been consumed.
    consume(TokenType::RIGHT_PAREN, "expect ')' after expression.");
  }
  void unary() {  // "Prefix" expression.
    const auto& prevToken = previous();
    const auto line = prevToken.line;
    parsePrecedence(Precedence::PREC_UNARY);  // Compile the operand.
    switch (prevToken.type) {
      case TokenType::MINUS: emitByte(OpCode::OP_NEGATE, line); break;
      case TokenType::BANG: emitByte(OpCode::OP_NOT, line); break;
      default: return;
    }
  }
  void binary(void) {  // "Infix" expression, the left operand has been consumed.
    const auto& prevToken = previous();
    const auto opType = prevToken.type;
    const auto line = prevToken.line;
    const auto rule = getRule(opType);
    /**
     * Each binary operator’s right-hand operand precedence is one level higher than its own, -
     * because the binary operators are left-associative. .e.g: 
     * 1 + 2 + 3 + 4 -> ((1 + 2) + 3) + 4.
    */
    parsePrecedence(static_cast<Precedence>(rule->precedence + 1)); 
    switch (opType) {
      case TokenType::PLUS: emitByte(OpCode::OP_ADD, line); break;
      case TokenType::MINUS: emitByte(OpCode::OP_SUBTRACT, line); break;
      case TokenType::STAR: emitByte(OpCode::OP_MULTIPLY, line); break;
      case TokenType::SLASH: emitByte(OpCode::OP_DIVIDE, line); break;
      case TokenType::BANG_EQUAL: emitByte(OpCode::OP_EQUAL); emitByte(OpCode::OP_NOT); break;
      case TokenType::EQUAL_EQUAL: emitByte(OpCode::OP_EQUAL); break;
      case TokenType::GREATER: emitByte(OpCode::OP_GREATER); break;
      case TokenType::GREATER_EQUAL: emitByte(OpCode::OP_LESS); emitByte(OpCode::OP_NOT); break;
      case TokenType::LESS: emitByte(OpCode::OP_LESS); break;
      case TokenType::LESS_EQUAL: emitByte(OpCode::OP_GREATER); emitByte(OpCode::OP_NOT); break;  // Not accurate under certain IEEE754 operations.
      default: return;
    }
  }
  void literal(void) {
    switch (previous().type) {
      case TokenType::FALSE: emitByte(OpCode::OP_FALSE); break;
      case TokenType::TRUE: emitByte(OpCode::OP_TRUE); break;
      case TokenType::NIL: emitByte(OpCode::OP_NIL); break;
      default: return;
    }
  }
  void expression(void) {
    parsePrecedence(Precedence::PREC_ASSIGNMENT);
  }
  Chunk compile(void) {
    expression();
    consume(TokenType::SOURCE_EOF, "expect end of expression.");
    emitByte(OpCode::OP_RETURN);  // End compiling.
#ifdef DEBUG_PRINT_CODE
    if (!Error::hadError) {
      ChunkDebugger::disassembleChunk(chunk, "code");
    }
#endif   
    tokens.clear();
    return chunk;
  }
};

#endif
