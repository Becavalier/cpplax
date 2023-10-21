#include "./compiler.h"

ClassCompiler* Compiler::currentClass = nullptr;
std::unordered_map<std::string_view, Token> Compiler::syntheticTokens = {
  { "this", { TokenType::THIS, "this", std::monostate {}, 0 } },
  { "super", { TokenType::SUPER, "super", std::monostate {}, 0 } }
};

