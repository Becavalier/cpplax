#ifndef	_COMPILER_H
#define	_COMPILER_H

#include <string>
#include "./chunk.h"
#include "./token.h"
#include "./error.h"

struct Compiler {
  const std::vector<Token>& tokens;
  Chunk chunk;
  explicit Compiler(const std::vector<Token>& tokens) : tokens(tokens) {}
  Chunk compile(void) {
    return Chunk {};
  }
};

#endif
