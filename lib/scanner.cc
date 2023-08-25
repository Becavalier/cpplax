#include "lib/scanner.h"

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

void Scanner::scanToken(void) {
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
