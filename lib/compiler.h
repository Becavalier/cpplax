#ifndef	_COMPILER_H
#define	_COMPILER_H

#define DEBUG_PRINT_CODE

#include <string>
#include <cstdlib>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include "./chunk.h"
#include "./token.h"
#include "./error.h"

#define UINT8_COUNT (UINT8_MAX + 1)

struct Compiler;
using typeParseFn = void (Compiler::*)(bool);

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
  InternedConstants* internedConstants;
  // locals.
  Local locals[UINT8_COUNT] = {};  // All the in-scope locals.
  size_t localCount = 0;  // Tracks how many locals are in scope.
  size_t scopeDepth = 0;  // The number of blocks surrounding the current bit of code we’re compiling.
  /**
   * Rule table for "Pratt Parser". The columns are:
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
    [TokenType::IDENTIFIER] = { &Compiler::variable, nullptr, Precedence::PREC_NONE },
    [TokenType::STRING] = { &Compiler::string, nullptr, Precedence::PREC_NONE },
    [TokenType::NUMBER] = { &Compiler::number, nullptr, Precedence::PREC_NONE },
    [TokenType::AND] = { nullptr, &Compiler::and_, Precedence::PREC_AND },
    [TokenType::CLASS] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::ELSE] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FALSE] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::FOR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::FN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::IF] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::NIL] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::OR] = { nullptr, &Compiler::or_, Precedence::PREC_OR },
    [TokenType::RETURN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SUPER] = { nullptr, nullptr, Precedence::PREC_NONE } ,
    [TokenType::THIS] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::TRUE] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::VAR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::WHILE] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SOURCE_EOF] = { nullptr, nullptr, Precedence::PREC_NONE },
  };
  explicit Compiler(std::vector<Token>& tokens) : tokens(tokens), current(tokens.cbegin()), objs(nullptr), internedConstants(new InternedConstants {}) {}
  Compiler(const Compiler&) = delete;
  auto& peek(void) const {
    return *current;
  }
  void advance(void) {
    ++current;
  }
  auto& previous(void) {
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
    if (panicMode) return;  // Minimize the number of cascaded compile errors that it reports.
    panicMode = true;
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
  void emitBytes(OpCodeType byteA, OpCodeType byteB) {
    emitByte(byteA);
    emitByte(byteB);
  }
  void emitByte(OpCodeType byte, size_t line) {
    chunk.addCode(byte, line);
  }
  void emitConstant(typeRuntimeValue value) {
    emitBytes(OpCode::OP_CONSTANT, makeConstant(value));
  }
  OpCodeType makeConstant(typeRuntimeValue value) {
    auto constantIdx = chunk.addConstant(value);
    if (constantIdx > UINT8_MAX) {
      errorAtPrevious("too many constants in one chunk.");
      constantIdx = 0;
    }
    return static_cast<OpCodeType>(constantIdx);
  }
  auto check(const TokenType& type) {
    return peek().type == type;
  }
  auto match(const TokenType& type) {
    if (!check(type)) return false;
    advance();
    return true;
  }
  void parsePrecedence(Precedence precedence) {  // Core driving function.
    advance();
    auto prefixRule = getRule(previous().type)->prefix;
    if (prefixRule == nullptr) {
      errorAtPrevious("expect expression.");
      return;
    }
    auto canAssign = precedence <= PREC_ASSIGNMENT;
    (this->*prefixRule)(canAssign);
    while (precedence <= getRule(peek().type)->precedence) {  // The prefix expression already compiled might be an operand for the infix operator.
      advance();
      auto infixRule = getRule(previous().type)->infix;
      (this->*infixRule)(canAssign);
    }
    if (canAssign && match(TokenType::EQUAL)) {
      errorAtPrevious("invalid assignment target.");
    }
  }
  const ParseRule* getRule(TokenType type) {
    return &rules[type];
  }
  void defineVariable(std::optional<OpCodeType> variable) {
    if (scopeDepth > 0) {
      locals[localCount - 1].initialized = true;
      return;
    }
    if (variable.has_value()) {
      emitBytes(OpCode::OP_DEFINE_GLOBAL, variable.value());  // OpCode for defining the variable and storing its initial value.
    }
  }
  auto identifierConstant(const Token& token) {
    // Take the given token and add its lexeme to the chunk’s constant table as a string.
    return makeConstant(internedConstants->add(token.lexeme, &objs));
  }
  void addLocal(const Token* name) {
    if (localCount == UINT8_COUNT) {
      errorAtCurrent("too many local variables in function.");
      return;
    }
    auto local = &locals[localCount++];
    local->name = name;
    local->depth = scopeDepth;
    local->initialized = false;
  }
  void declareVariable(void) {
    if (scopeDepth == 0) return;

    // Add the local variable to the compiler’s list of variables in the current scope.
    const auto& name = previous();

    // Detect the error that having two variables with the same name in the same local scope.
    for (auto i = localCount; i >= 1; i--) {
      auto local = &locals[i - 1];
      if (local->initialized && local->depth < scopeDepth) {
        break;
      }
      if (local->name->lexeme == name.lexeme) {
        errorAtPrevious("already a variable with this name in this scope.");
      }
    }
    addLocal(&name);
  }
  auto parseVariable(const char* errorMsg) {
    consume(TokenType::IDENTIFIER, errorMsg);
    declareVariable();
    // Locals aren’t looked up by name. 
    return scopeDepth == 0 
      ? std::make_optional(identifierConstant(previous())) 
      : std::nullopt;
  }
  std::optional<OpCodeType> resolveLocal(const Token& name) {
    for (auto i = localCount; i >= 1; i--) {
      const auto idx = i - 1;
      const auto local = &locals[idx];
      if (name.lexeme == local->name->lexeme) {  // Find the last declared variable with the identifier.
        if (!local->initialized) {
          errorAtPrevious("can't read local variable in its own initializer.");
        }
        return std::make_optional(idx);
      }
    }
    return std::nullopt;
  }
  void namedVariable(const Token& name, bool canAssign) {
    OpCodeType setOp, getOp, variableIdx;
    const auto local = resolveLocal(name);
    if (local.has_value()) {
      variableIdx = local.value(); 
      getOp = OpCode::OP_GET_LOCAL;
      setOp = OpCode::OP_SET_LOCAL;
    } else {
      variableIdx = identifierConstant(name);
      getOp = OpCode::OP_GET_GLOBAL;
      setOp = OpCode::OP_SET_GLOBAL;
    }
    if (canAssign && match(TokenType::EQUAL)) {
      expression();
      emitBytes(setOp, variableIdx);
    } else {
      emitBytes(getOp, variableIdx);
    }
  }
  void variable(bool canAssign) {
    /**
     * This function should look for and consume the = only if, - 
     * it’s in the context of a low-precedence expression, to avoid case like below:
     * a * b = c + d;
    */
    namedVariable(previous(), canAssign);
  }
  void string(bool) {
    const auto str = std::get<std::string_view>(previous().literal);
    emitConstant(internedConstants->add(str, &objs));
  }
  void number(bool) {
    emitConstant(previous().literal);  // Number constant has been consumed.
  }
  void grouping(bool) {
    expression();  // The opening '(' has been consumed.
    consume(TokenType::RIGHT_PAREN, "expect ')' after expression.");
  }
  void unary(bool) {  // "Prefix" expression.
    const auto& prevToken = previous();
    const auto line = prevToken.line;
    parsePrecedence(Precedence::PREC_UNARY);  // Compile the operand.
    switch (prevToken.type) {
      case TokenType::MINUS: emitByte(OpCode::OP_NEGATE, line); break;
      case TokenType::BANG: emitByte(OpCode::OP_NOT, line); break;
      default: return;
    }
  }
  void binary(bool) {  // "Infix" expression, the left operand has been consumed.
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
  void literal(bool) {
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
  void expressionStatement(void) {
    // Simply an expression followed by a semicolon.
    expression();
    consume(TokenType::SEMICOLON, "expect ';' after expression.");
    emitByte(OpCode::OP_POP);  // Discard the result.
  }
  void block(void) {
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::SOURCE_EOF)) {
      declaration();
    }
    consume(TokenType::RIGHT_BRACE, "expect '}' after block.");
  }
  void beginScope(void) {
    scopeDepth++;
  }
  void endScope(void) {
    scopeDepth--;
    while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) {
      emitByte(OpCode::OP_POP);
      localCount--;
    }
  }
  auto emitJump(OpCodeType instruction) {
    emitByte(instruction);
    emitByte(0xff);  // Set placeholder operands.
    emitByte(0xff);
    return static_cast<size_t>(chunk.count() - 2);  // Return the position to the placeholder.
  }
  void emitLoop(size_t loopStart) {
    // Unconditionally jumps backwards by a given offset.
    emitByte(OpCode::OP_LOOP);
    auto offset = chunk.count() - loopStart + 2;
    if (offset > UINT16_MAX) errorAtCurrent("loop body too large.");
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
  }
  void patchJump(size_t offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    auto jump = chunk.count() - offset - 2; 
    if (jump > UINT16_MAX) {
      errorAtCurrent("too much code to jump over.");
    }
    chunk.code[offset] = (jump >> 8) & 0xff;  // Higer 8-bits offset.
    chunk.code[offset + 1] = jump & 0xff;  // Lower 8-bits offset.
  }
  void ifStatement(void) {
    consume(TokenType::LEFT_PAREN, "expect '(' after 'if'.");
    expression();  // Compile the condition expression, leave the condition value on the stack.
    consume(TokenType::RIGHT_PAREN, "expect ')' after condition.");
    auto thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);  // Pop the condition value before "then" branch.
    statement();
    auto elseJump = emitJump(OpCode::OP_JUMP);
    patchJump(thenJump);  // Backpatching.
    emitByte(OpCode::OP_POP);  // Pop the condition value before "else" branch.
    if (match(TokenType::ELSE)) statement();
    patchJump(elseJump);
  }
  void or_(bool) {
    auto elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    auto endJump = emitJump(OpCode::OP_JUMP);
    patchJump(elseJump);
    emitByte(OpCode::OP_POP);
    parsePrecedence(Precedence::PREC_OR);
    patchJump(endJump);
  }
  void and_(bool) {
    auto endJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);
    parsePrecedence(Precedence::PREC_AND);  // Parse infix "and" with its right operand.
    patchJump(endJump);
  }
  void whileStatement(void) {
    auto loopStart = chunk.count();
    consume(TokenType::LEFT_PAREN, "expect '(' after 'while'.");
    expression();
    consume(TokenType::RIGHT_PAREN, "expect ')' after condition.");
    auto exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OpCode::OP_POP);
  }
  void forStatement(void) {
    beginScope();
    consume(TokenType::LEFT_PAREN, "expect '(' after 'for'.");
    if (match(TokenType::VAR)) {
      varDeclaration();
    } else if (!match(TokenType::SEMICOLON)) {
      expressionStatement();
    }

    auto loopStart = chunk.count();
    std::optional<size_t> exitJump;
    if (!match(TokenType::SEMICOLON)) {  // Condition clause.
      expression();
      consume(TokenType::SEMICOLON, "eßxpect ';' after loop condition.");
      // Jump out of the loop if the condition is false.
      exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
      emitByte(OpCode::OP_POP);
    }

    if (!match(TokenType::RIGHT_PAREN)) {
      auto bodyJump = emitJump(OpCode::OP_JUMP);
      auto incrementStart = chunk.count();
      expression();
      emitByte(OpCode::OP_POP);
      consume(TokenType::RIGHT_PAREN, "expect ')' after for clauses.");

      emitLoop(loopStart);
      loopStart = incrementStart;
      patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump.has_value()) {
      patchJump(exitJump.value());
      emitByte(OpCode::OP_POP);
    }

    endScope();
  }
  void statement(void) {
    if (match(TokenType::IF)) {
      ifStatement();
    } else if (match(TokenType::FOR)) {
      forStatement();
    } else if (match(TokenType::WHILE)) {
      whileStatement();
    } else if (match(TokenType::LEFT_BRACE)) {
      beginScope();
      block();
      endScope();
    } else {
      expressionStatement();
    }
  }
  void varDeclaration(void) {
    auto varConstantIdx = parseVariable("expect variable name.");  // Emit variable name as constant.
    if (match(TokenType::EQUAL)) {
      expression();
    } else {
      emitByte(OpCode::OP_NIL);
    }
    consume(TokenType::SEMICOLON, "expect ';' after variable declaration.");
    defineVariable(varConstantIdx);
  }
  void declaration(void) {
    if (match(TokenType::VAR)) {
      varDeclaration();
    } else {
      statement();
    }
    if (panicMode) synchronize();
  }
  void synchronize(void) {
    panicMode = false;
    while (peek().type != TokenType::SOURCE_EOF) {
      if (previous().type == TokenType::SEMICOLON) return;
      switch (peek().type) {
        case TokenType::CLASS:
        case TokenType::FN:
        case TokenType::VAR:
        case TokenType::FOR:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::RETURN:
          return;
        default: ;
      }
      advance();
    }
  }
  void endCompiler(void) {
    emitByte(OpCode::OP_RETURN);  // End compiling.
  }
  Chunk compile(void) {
    while (!match(TokenType::SOURCE_EOF)) {
      declaration();
    }
    endCompiler();
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
