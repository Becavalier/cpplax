#ifndef	_COMPILER_H
#define	_COMPILER_H

#include <string>
#include <cstdlib>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include "./macro.h"
#include "./chunk.h"
#include "./token.h"
#include "./error.h"
#include "./constant.h"
#include "./object.h"
#include "./memory.h"

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

struct ClassCompiler {
  struct ClassCompiler* enclosing;
};

struct ParseRule {
  typeParseFn prefix;
  typeParseFn infix;
  Precedence precedence;
};

struct Local {
  const Token* name;
  size_t depth;
  bool isCaptured = false;
  bool initialized = false;
};

struct Upvalue {
  uint8_t index;  // Which local slot the upvalue is capturing.
  bool isLocal;
};

struct Compiler {
  Memory* mem;
  FuncObj* compilingFunc = nullptr;
  FunctionScope compilingScope = FunctionScope::TYPE_TOP_LEVEL;
  InternedConstants* internedConstants;
  Upvalue upvalues[UINT8_COUNT] = {};
  Local locals[UINT8_COUNT] = {};  // All the in-scope locals.
  size_t localCount = 0;  // Tracks how many locals are in scope.
  size_t scopeDepth = 0;  // The number of blocks surrounding the current bit of code we’re compiling.
  std::vector<Token>::const_iterator current;
  std::vector<Token>& tokens;
  Compiler* enclosing;
  static ClassCompiler* currentClass;  // Point to a struct representing the current, innermost class being compiled.
  /**
   * Rule table for "Pratt Parser". The columns are:
   * - The function to compile a prefix expression starting with a token of that type.
   * - The function to compile an infix expression whose left operand is followed by a token of that type.
   * - The precedence of an infix expression that uses that token as an operator.
  */
  const ParseRule rules[TokenType::TOTAL] = {
    [TokenType::LEFT_PAREN] = { &Compiler::grouping, &Compiler::call, Precedence::PREC_CALL },
    [TokenType::RIGHT_PAREN] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::LEFT_BRACE] = { nullptr, nullptr, Precedence::PREC_NONE }, 
    [TokenType::RIGHT_BRACE] = {nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::COMMA] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::DOT] = { nullptr, &Compiler::dot, Precedence::PREC_CALL },
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
    [TokenType::THIS] = { &Compiler::this_, nullptr, Precedence::PREC_NONE },
    [TokenType::TRUE] = { &Compiler::literal, nullptr, Precedence::PREC_NONE },
    [TokenType::VAR] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::WHILE] = { nullptr, nullptr, Precedence::PREC_NONE },
    [TokenType::SOURCE_EOF] = { nullptr, nullptr, Precedence::PREC_NONE },
  };
  Compiler(
    std::vector<Token>& tokens, 
    std::vector<Token>::const_iterator tokenIt,
    Memory* memPtr,
    InternedConstants* constants = nullptr, 
    FunctionScope scope = FunctionScope::TYPE_TOP_LEVEL, 
    Compiler* enclosingCompiler = nullptr) : 
    mem(memPtr),
    compilingFunc(memPtr->makeObj<FuncObj>()),
    compilingScope(scope),
    internedConstants(constants), 
    current(tokenIt),
    tokens(tokens),
    enclosing(enclosingCompiler) {
      if (scope != FunctionScope::TYPE_TOP_LEVEL) {
        compilingFunc->name = internedConstants->add(previous().lexeme)->cast<StringObj>();
      }
      Local* local = &locals[localCount++];  // Stack slot zero is for VM’s own internal use.
      local->depth = 0;
      if (scope == FunctionScope::TYPE_METHOD || scope == FunctionScope::TYPE_INITIALIZER) {
        local->name = &tokens.back();
        local->initialized = true;
      } else {
        local->name = nullptr;  // Empty name.
      }
    }
  Compiler(const Compiler&) = delete;
  Compiler(const Compiler&&) = delete; 
  auto& currentChunk(void) {
    return compilingFunc->chunk;
  }
  const Token& peek(void) const {
    return *current;
  }
  const Token& previous(void) {
    return *(current - 1);
  }
  void advance(void) {
    ++current;
  }
  void consume(TokenType type, const char* msg) {
    if (peek().type == type) {
      advance();
      return;
    }
    errorAtCurrent(msg);
  }
  void errorAtCurrent(const char* msg) {
    throw TokenError { peek(), msg };
  }
  void errorAtPrevious(const char* msg) {
    throw TokenError { previous(), msg };  // Report an error at the location of the just consumed token.
  }
  void emitByte(OpCodeType byte) {
    currentChunk().addCode(byte, previous().line);
  }
  void emitBytes(OpCodeType byteA, OpCodeType byteB) {
    emitByte(byteA);
    emitByte(byteB);
  }
  void emitByte(OpCodeType byte, size_t line) {
    currentChunk().addCode(byte, line);
  }
  void emitReturn(void) {
    // Load the class instance onto the stack if it was returned from an initializer call.
    if (compilingScope == FunctionScope::TYPE_INITIALIZER) {
      emitBytes(OpCode::OP_GET_LOCAL, 0);
    } else {
      emitByte(OpCode::OP_NIL);
    }
    emitByte(OpCode::OP_RETURN);
  }
  void emitConstant(const typeRuntimeValue& value) {
    emitBytes(OpCode::OP_CONSTANT, makeConstant(value));
  }
  OpCodeType makeConstant(const typeRuntimeValue& value) {
    auto constantIdx = currentChunk().addConstant(value);
    if (constantIdx > UINT8_MAX) {
      errorAtPrevious("too many constants in one chunk.");
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
    }
    const auto canAssign = precedence <= PREC_ASSIGNMENT;
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
  void markInitialized(void) {
    if (scopeDepth == 0) return;
    locals[localCount - 1].initialized = true;
  }
  /**
   * Mark the local variable as "initialized", or save it as a global at runtime.
  */
  void defineVariable(std::optional<OpCodeType> varIdx) {
    if (scopeDepth > 0) {
      markInitialized();
      return;
    }
    if (varIdx.has_value()) {
      emitBytes(OpCode::OP_DEFINE_GLOBAL, varIdx.value());  // OpCode for defining the variable and storing its initial value.
    }
  }
  /**
   * Take the given token and add its lexeme to the chunk’s constant table as a string.
  */
  auto identifierConstant(const Token& token) {
    return makeConstant(internedConstants->add(token.lexeme));
  }
  void addLocal(const Token* name) {
    if (localCount == UINT8_COUNT) {
      errorAtCurrent("too many local variables in function.");
    }
    auto& local = locals[localCount++];
    local.name = name;
    local.depth = scopeDepth;
  }
  /**
   * Add variable as a local, and detect certain errors.
  */
  void declareVariable(void) {
    if (scopeDepth == 0) return;
    const auto& name = previous();

    // Detect the error that having two variables with the same name in the same local scope.
    for (auto i = localCount; i >= 1; i--) {
      auto local = &locals[i - 1];
      if ((local->initialized && local->depth < scopeDepth) || local->name == nullptr) {
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
      if (local->name != nullptr && (name.lexeme == local->name->lexeme)) {  // Find the last declared variable with the identifier.
        if (!local->initialized) {
          errorAtPrevious("can't read local variable in its own initializer.");
        }
        return std::make_optional(idx);
      }
    }
    return std::nullopt;
  }
  OpCodeType addUpvalue(OpCodeType index, bool isLocal) {
    const auto upvalueCount = compilingFunc->upvalueCount;

    // Search and reuse the existing upvalues.
    for (uint32_t i = 0; i < upvalueCount; i++) {
      auto upvalue = &upvalues[i];
      if (upvalue->index == index && upvalue->isLocal == isLocal) {
        return i;
      }
    }
    if (upvalueCount == UINT8_COUNT) {
      errorAtCurrent("too many closure variables in function.");
    }
    upvalues[upvalueCount].isLocal = isLocal;
    upvalues[upvalueCount].index = index;
    return compilingFunc->upvalueCount++;  // Return the index of the upvalue.
  }
  std::optional<OpCodeType> resolveUpvalue(const Token& name) {
    /**
     * Compiler: upvalue [isLocal, index].
     * VM: UpvalueObj [next, location], will be managed in a sorted linked list.
     * 
                          inner.upvalue <= middle.upvalue
                            │                          │
                            ▼                          │
              outer()     middle()     inner()         |
            ┌─┬─────┐  ┌─┬───────┐  ┌─┬───────┐        │
            │0│local│◄─┤0│upvalue│  │0│upvalue│        │
            └─┴─────┘  └─┴───────┘  └─┴───────┘        │
                  ▲      isLocal: 1   isLocal: 0 ──────┘
                  │      index: 0 │   index: 0
                  │               │
      vm: middle.upvalue <= *(outer.slot + index)
    */
    if (enclosing == nullptr) return std::nullopt;

    // Looking for the variable in the enclosing function.
    const auto local = enclosing->resolveLocal(name);
    if (local.has_value()) {
      const auto localIdx = local.value();
      enclosing->locals[localIdx].isCaptured = true;
      return addUpvalue(localIdx, true);
    }
    /**
     * This series of "resolveUpvalue()" calls works its way along the chain of nested compilers -
     * until it hits one of the base cases — either it finds an actual local variable to capture or it runs out of compilers.
    */
    const auto upvalue = enclosing->resolveUpvalue(name);
    if (upvalue.has_value()) {
      return addUpvalue(upvalue.value(), false);
    }
    return std::nullopt;
  }
  void namedVariable(const Token& name, bool canAssign) {
    OpCodeType setOp, getOp, varIndex;
    auto local = resolveLocal(name);
    if (local.has_value()) {
      // Looking for a local variable declared in the current function's scope.
      varIndex = local.value(); 
      getOp = OpCode::OP_GET_LOCAL;
      setOp = OpCode::OP_SET_LOCAL;
    } else if ((local = resolveUpvalue(name)).has_value()) {  // Returning the "upvalue index".
      // Looking for a local variable declared in any of the surrounding functions.
      varIndex = local.value(); 
      getOp = OpCode::OP_GET_UPVALUE;
      setOp = OpCode::OP_SET_UPVALUE;
    } else {
      // Looking for a local variable declared in the top-level function.
      varIndex = identifierConstant(name);
      getOp = OpCode::OP_GET_GLOBAL;
      setOp = OpCode::OP_SET_GLOBAL;
    }
    if (canAssign && match(TokenType::EQUAL)) {
      expression();
      emitBytes(setOp, varIndex);
    } else {
      emitBytes(getOp, varIndex);
    }
  }
  void this_(bool) {
    if (currentClass == nullptr) {
      errorAtCurrent("can't use 'this' outside of a class.");
    }
    variable(false);  // Treat "this" as a lexically scoped local variable whose value gets initialized.
  }
  void variable(bool canAssign) {
    /**
     * This function should look for and consume the "=" only if, - 
     * it’s in the context of a low-precedence expression, to avoid case like below:
     * a * b = c + d;
    */
    namedVariable(previous(), canAssign);
  }
  void string(bool) {
    const auto str = std::get<std::string_view>(previous().literal);
    emitConstant(internedConstants->add(str));
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
  auto argumentList(void) {
    uint8_t argCount = 0;
    if (!check(TokenType::RIGHT_PAREN)) {
      do {
        expression();
        if (argCount == 255) {
          errorAtCurrent("can't have more than 255 arguments.");
        }
        argCount++;
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "expect ')' after arguments.");
    return argCount;
  }
  void dot(bool canAssign) {
    consume(TokenType::IDENTIFIER, "expect property name after '.'.");
    const auto name = identifierConstant(previous());
    if (canAssign && match(TokenType::EQUAL)) {  
      expression();
      emitBytes(OpCode::OP_SET_PROPERTY, name);
    } else if(match(TokenType::LEFT_PAREN)) {
      const auto argCount = argumentList();
      emitBytes(OpCode::OP_INVOKE, name);
      emitByte(argCount);
    } else {
      emitBytes(OpCode::OP_GET_PROPERTY, name);
    }
  }
  void call(bool) {
    auto argCount = argumentList();
    emitBytes(OpCode::OP_CALL, argCount);
  }
  void expression(void) {
    parsePrecedence(Precedence::PREC_ASSIGNMENT);  // Start with a relatively lower precedence.
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
      // Closed locals will be hoisted onto the heap.
      emitByte(locals[localCount - 1].isCaptured ? OpCode::OP_CLOSE_UPVALUE : OpCode::OP_POP);
      localCount--;
    }
  }
  auto emitJump(OpCodeType instruction) {
    emitByte(instruction);
    emitByte(0xff);  // Set placeholder operands.
    emitByte(0xff);
    return static_cast<size_t>(currentChunk().count() - 2);  // Return the position to the placeholder.
  }
  void emitLoop(size_t loopStart) {
    // Unconditionally jumps backwards by a given offset.
    emitByte(OpCode::OP_LOOP);
    auto offset = currentChunk().count() - loopStart + 2;
    if (offset > UINT16_MAX) errorAtCurrent("loop body too large.");
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
  }
  void patchJump(size_t offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    auto jump = currentChunk().count() - offset - 2; 
    if (jump > UINT16_MAX) {
      errorAtCurrent("too much code to jump over.");
    }
    currentChunk().code[offset] = (jump >> 8) & 0xff;  // Higer 8-bits offset.
    currentChunk().code[offset + 1] = jump & 0xff;  // Lower 8-bits offset.
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
    auto loopStart = currentChunk().count();
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
    auto loopStart = currentChunk().count();
    std::optional<size_t> exitJump;
    if (!match(TokenType::SEMICOLON)) {  // Condition clause.
      expression();
      consume(TokenType::SEMICOLON, "expect ';' after loop condition.");
      // Jump out of the loop if the condition is false.
      exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
      emitByte(OpCode::OP_POP);
    }
    if (!match(TokenType::RIGHT_PAREN)) {
      auto bodyJump = emitJump(OpCode::OP_JUMP);
      auto incrementStart = currentChunk().count();
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
  void returnStatement(void) {
    if (compilingScope == FunctionScope::TYPE_TOP_LEVEL) {
      errorAtPrevious("can't return from top-level code.");
    }
    if (match(TokenType::SEMICOLON)) {
      emitReturn();
    } else {
      if (compilingScope == FunctionScope::TYPE_INITIALIZER) {
        errorAtCurrent("can't return a value from an initializer.");
      }
      expression();
      consume(TokenType::SEMICOLON, "expect ';' after return value.");
      emitByte(OpCode::OP_RETURN);
    }
  }
  void statement(void) {
    if (match(TokenType::IF)) {
      ifStatement();
    } else if (match(TokenType::RETURN)) {
      returnStatement();
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
  auto functionCore(void) {
    beginScope();
    consume(TokenType::LEFT_PAREN, "expect '(' after function name.");
    if (!check(TokenType::RIGHT_PAREN)) {
      do {
        compilingFunc->arity++;
        if (compilingFunc->arity > 255) {
          errorAtCurrent("can't have more than 255 parameters.");
        }
        defineVariable(parseVariable("expect parameter name."));
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "expect '{' before function body.");
    block();
    return endCompiler();
  }
  void function(FunctionScope scope) {
    Compiler compiler { tokens, current, mem, internedConstants, scope, this };
    const auto compiledFunc = compiler.functionCore();
    current = compiler.current;  // Update compiling function.
    if (compiledFunc->upvalueCount > 0) {
      emitBytes(OpCode::OP_CLOSURE, makeConstant(compiledFunc));
      for (uint32_t i = 0; i < compiledFunc->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);  // local or upvalue.
        emitByte(compiler.upvalues[i].index);  // local slot or upvalue index.
      }
    } else {
      emitBytes(OpCode::OP_CONSTANT, makeConstant(compiledFunc));
    }
  }
  void funDeclaration(void) {
    auto varIdx = parseVariable("expect function name.");
    markInitialized();
    function(FunctionScope::TYPE_BODY);
    defineVariable(varIdx);
  }
  void method(void) {
    consume(TokenType::IDENTIFIER, "expect method name.");
    const auto constant = identifierConstant(previous());
    auto scope = FunctionScope::TYPE_METHOD;
    if (previous().lexeme == INITIALIZER_NAME) {
      scope = FunctionScope::TYPE_INITIALIZER;
    }
    function(scope);
    emitBytes(OpCode::OP_METHOD, constant);
  }
  void classDeclaration(void) {
    consume(TokenType::IDENTIFIER, "expect class name.");
    const auto& className = previous();
    const auto nameConstant = identifierConstant(className);  // Add the name to the surrounding function’s constant table.
    declareVariable();
    emitBytes(OpCode::OP_CLASS, nameConstant);  // Create runtime representation.
    defineVariable(nameConstant);  // Mark local or add the runtime entry to the global store.

    // Add the compiling class to the class chain.
    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    namedVariable(className, false);  // Load the class onto the stack.
    consume(TokenType::LEFT_BRACE, "expect '{' before class body.");
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::SOURCE_EOF)) {
      method();
    }
    consume(TokenType::RIGHT_BRACE, "expect '}' after class body.");
    emitByte(OpCode::OP_POP);
    currentClass = currentClass->enclosing;  // Update the class compiling chain.
  }
  void declaration(void) {
    try {
      if (match(TokenType::FN)) {
        funDeclaration();
      } else if (match(TokenType::CLASS)) {
        classDeclaration();
      } else if (match(TokenType::VAR)) {
        varDeclaration();
      } else {
        statement();
      }
    } catch(TokenError& err) {
      Error::error(err.token, err.msg);  // Error::hadError -> true.
      synchronize();
    }
  }
  void synchronize(void) {
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
  FuncObj* endCompiler(void) {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    ChunkDebugger::disassembleChunk(currentChunk(), compilingFunc->name != nullptr ? compilingFunc->name->str.data() : "<script>");
#endif 
    return compilingFunc;
  }
  auto compile(void) {
    mem->setCompiler(this);
    while (!match(TokenType::SOURCE_EOF)) {
      declaration();
    }
    return endCompiler();
  }
};

#endif
