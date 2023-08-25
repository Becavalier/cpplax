#ifndef	_SCANNER_H
#define	_SCANNER_H

#include <vector>
#include <variant>
#include <cctype>
#include <string>
#include <unordered_map>
#include "lib/token.h"

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
  inline bool isAtEnd(void) const {
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
  inline bool isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
  }
  inline bool isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
  }
  inline bool isAlphaNumeric(char c) const {
    return isDigit(c) || isAlpha(c);
  }
  inline char peekNext(void) {
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
  inline void scanToken(void) {
    char c = advance();
    switch (c) {
      case '(': addToken(TokenType::LEFT_PAREN); break;
      case ')': addToken(TokenType::RIGHT_PAREN); break;
      case '{': addToken(TokenType::LEFT_BRACE); break;
      case '}': addToken(TokenType::RIGHT_BRACE); break;
      case ',': addToken(TokenType::COMMA); break;
      case '.': addToken(TokenType::DOT); break;
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
          Error::error(line, "unexpected character.");  // Report unrecognized tokens.
        }
        break;
      }
    }
  }
};

const typeKeywordList Scanner::keywords =  {
  { "and", TokenType::AND },
  { "class", TokenType::CLASS },
  { "else", TokenType::ELSE },
  { "false", TokenType::FALSE },
  { "for", TokenType::FOR },
  { "fun", TokenType::FUN },
  { "if", TokenType::IF },
  { "nil", TokenType::NIL },
  { "or", TokenType::OR },
  { "print", TokenType::PRINT },
  { "return", TokenType::RETURN },
  { "super", TokenType::SUPER },
  { "this", TokenType::THIS },
  { "true", TokenType::TRUE },
  { "var", TokenType::VAR },
  { "while", TokenType::WHILE },
};

#endif
