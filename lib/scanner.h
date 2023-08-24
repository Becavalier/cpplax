#ifndef	_SCANNER_H
#define	_SCANNER_H

#include <vector>
#include <variant>
#include "lib/token.h"

class Scanner {
  std::string source;
  std::vector<Token> tokens;
  std::string::const_iterator start;  // Points to the first char in the lexeme.
  std::string::const_iterator current;  // Points at the character currently being considered.
  int line = 1;
 public:
  Scanner(const std::string& code) : source(code) {};
  std::vector<Token> scanTokens(void) {
    while (current != cend(source)) {
      start = current;
      scanToken();
    }
    // End of file.
    tokens.emplace_back(TokenType::EOF, "", std::monostate {}, line);
    return tokens;
  }
  void scanToken(void) {

  }
};

#endif
