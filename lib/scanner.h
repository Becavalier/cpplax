#ifndef	_SCANNER_H
#define	_SCANNER_H

#include <vector>
#include <variant>
#include <cctype>
#include <string>
#include <unordered_map>
#include "lib/token.h"
#include "lib/error.h"

using typeKeywordList = std::unordered_map<std::string, TokenType>;
class Scanner {
  std::string source;
  std::vector<Token> tokens;
  std::string::const_iterator start;  // Points to the first char in the lexeme.
  std::string::const_iterator current;  // Points at the character currently being considered.
  int line = 1;
  static const typeKeywordList keywords;
 public:
  Scanner(const std::string& code) : source(code) {};
  bool isAtEnd(void) const {
    return current == cend(source);
  }
  std::vector<Token> scanTokens(void) {
    current = cbegin(source);
    while (!isAtEnd()) {
      start = current;
      scanToken();
    }
    // Mark the end of file.
    tokens.emplace_back(TokenType::EOF, "", std::monostate {}, line);
    return tokens;
  }
  char advance(void) {
    return *current++;  // Return current character and step ahead.
  }
  void addToken(TokenType type) {
    addToken(type, std::monostate {});
  }
  void addToken(TokenType type, Token::typeLiteral literal) {
    std::string text { start, current };
    tokens.emplace_back(type, text, literal, line);
  }
  bool forwardMatch(char expected) {
    // Look ahead to see if it could match another token type (the combination one).
    if (isAtEnd()) return false;
    if (*current != expected) return false;
    current++;
    return true;
  }
  bool isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
  }
  bool isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
  }
  bool isAlphaNumeric(char c) const {
    return isDigit(c) || isAlpha(c);
  }
  char peekNext(void) {
    if (current + 1 == cend(source)) return '\0';
    return *(current + 1); 
  }
  void scanString(void) {
    while (!isAtEnd() && *current != '"') {
      if (*current == '\n') line++;  // Support multi-line strings.
      advance();
    }
    if (isAtEnd()) {
      Error::error(line, "unterminated string.");
      return;
    }
    advance();  // Catch the closing quote.
    std::string value = std::string { start + 1, current - 1 };
    addToken(TokenType::STRING, value);
  }
  void scanNumber(void) {
    while (isDigit(*current)) advance();
    if (*current == '.' && isDigit(peekNext())) {  // Find the fractional part (if any).
      advance();
      while (isDigit(*current)) advance();  // Keep consuming till the last digit.
    }
    addToken(TokenType::NUMBER, std::stod(std::string { start, current }));
  }
  void scanIdentifier(void) {
    while (isAlphaNumeric(*current)) advance();
    // Check if it's a keyword.
    auto text = std::string { start, current };
    auto type = Scanner::keywords.find(text);
    if (type == Scanner::keywords.end()) {
      addToken(TokenType::IDENTIFIER);
    } else {
      addToken(type->second);  // Change token type.
    }
  }
  void scanToken(void);
};

#endif
