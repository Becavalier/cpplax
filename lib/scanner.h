#ifndef	_SCANNER_H
#define	_SCANNER_H

#include <vector>
#include <variant>
#include <cctype>
#include <string>
#include <unordered_map>
#include "./token.h"
#include "./error.h"
#include "./type.h"

class Scanner {
  size_t line = 1;
  const std::string& source;
  std::vector<Token> tokens;
  std::string::const_iterator start;  // Points to the first char in the lexeme.
  std::string::const_iterator current;  // Points at the character currently being considered.
  static const typeKeywordList keywords;
 public:
  explicit Scanner(const std::string& code) : source(code) {};
  bool isAtEnd(void) const {
    return current == cend(source);
  }
  std::vector<Token> scanTokens(void) {
    current = cbegin(source);
    while (!isAtEnd()) {
      start = current;  // Mark the beginning of the next token.
      scanToken();
    }
    tokens.emplace_back(TokenType::SOURCE_EOF, "", std::monostate {}, line);  // Mark the end of file.
    return tokens;
  }
  char advance(void) {
    return *current++;  // Return current character and step ahead.
  }
  void addToken(TokenType type) {
    addToken(type, std::monostate {});
  }
  void addToken(TokenType type, Token::typeLiteral literal) {
    tokens.emplace_back(type, std::string_view { start, current }, literal, line);
  }
  bool forwardMatch(char expected) {
    // Look ahead to see if it could match another token type (the combination pair).
    if (isAtEnd()) return false;
    if (*current != expected) return false;
    ++current;
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
  char peekNext(void) const {
    if (current + 1 == cend(source)) return '\0';
    return *(current + 1); 
  }
  void scanString(void) {
    while (true) {
      if (isAtEnd() || (*current == '"' && *(current - 1) != '\\')) break;
      if (*current == '\n') line++;  // Support multi-line strings.
      advance();
    }
    if (isAtEnd()) {
      Error::error(line, "unterminated string.");
      return;
    }
    advance();  // Catch the closing quote.
    addToken(TokenType::STRING, std::string_view { start + 1, current - 1 });
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
    while (!isAtEnd() && isAlphaNumeric(*current)) advance();
    // Check if it's a keyword.
    auto text = std::string_view { start, current };
    auto type = Scanner::keywords.find(text);
    addToken(type == Scanner::keywords.end() ? TokenType::IDENTIFIER : type->second);
  }
  void scanToken(void) {
    char c = advance();
    switch (c) {
      case '(': addToken(TokenType::LEFT_PAREN); break;
      case ')': addToken(TokenType::RIGHT_PAREN); break;
      case '{': addToken(TokenType::LEFT_BRACE); break;
      case '}': addToken(TokenType::RIGHT_BRACE); break;
      case ',': addToken(TokenType::COMMA); break;
      case '.': {
        if (isDigit(peekNext())) scanNumber();
        else addToken(TokenType::DOT);
        break;
      }
      case '-': addToken(TokenType::MINUS); break;
      case '+': addToken(TokenType::PLUS); break;
      case ';': addToken(TokenType::SEMICOLON); break;
      case '*': addToken(TokenType::STAR); break; 
      case '!': addToken(forwardMatch('=') ? TokenType::BANG_EQUAL : TokenType::BANG); break;
      case '=': addToken(forwardMatch('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
      case '<': addToken(forwardMatch('=') ? TokenType::LESS_EQUAL : TokenType::LESS); break;
      case '>': addToken(forwardMatch('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
      case '/': {
        if (forwardMatch('/')) {
          // A comment goes until the end of the line, then we skip the current line.
          while (!isAtEnd() && *current != '\n') {
            advance();
          }
        } else if (forwardMatch('*')) {
          while (!isAtEnd()) {
            if (*current == '\n') line++;
            if (*current != '*' || peekNext() != '/') {
              advance();
            } else {
              current += 2;  // Skip the latter part of the comment pair.
              break;
            }
          }
        } else { 
          addToken(TokenType::SLASH);
        }
        break;
      }
      case ' ':
      case '\r':
      case '\t': break;
      case '\n': line++; break;
      case '"': scanString(); break;
      default: {
        if (isDigit(c)) {
          scanNumber();
        } else if (isAlpha(c)) {
          scanIdentifier();
        } else {
          // TODO: coalescing a run of invalid characters into a single error.
          Error::error(line, c, "unexpected character.");
        }
        break;
      }
    }
  }
};

#endif
