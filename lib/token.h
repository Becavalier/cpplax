
#ifndef	_TOKEN_H
#define	_TOKEN_H

#include <type_traits>
#include <string>
#include <utility>
#include <variant>

#undef EOF

enum class TokenType : unsigned char {
  // Single-character tokens.
  LEFT_PAREN, 
  RIGHT_PAREN, 
  LEFT_BRACE, 
  RIGHT_BRACE,
  COMMA, 
  DOT, 
  MINUS, 
  PLUS, 
  SEMICOLON, 
  SLASH, 
  STAR,

  // One or two character tokens.
  BANG, 
  BANG_EQUAL,
  EQUAL, 
  EQUAL_EQUAL,
  GREATER, 
  GREATER_EQUAL,
  LESS, 
  LESS_EQUAL,

  // Literals.
  IDENTIFIER, 
  STRING, 
  NUMBER,

  // Keywords.
  AND, 
  CLASS, 
  ELSE, 
  FALSE, 
  FUN, 
  FOR, 
  IF, 
  NIL, 
  OR,
  PRINT, 
  RETURN, 
  SUPER, 
  THIS, 
  TRUE, 
  VAR, 
  WHILE, 
  EOF
};

class Token {
  using typeLiteral = std::variant<std::monostate, std::string, double>;
  TokenType type;
  std::string lexeme;
  typeLiteral literal;
  int line;
 public:
  friend std::ostream& operator<<(std::ostream& os, const Token& token);
  Token(TokenType type, std::string lexeme, typeLiteral literal, int line) 
    : type(type), lexeme(std::move(lexeme)), literal(literal), line(line) {}
};

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os << static_cast<std::underlying_type<TokenType>::type>(token.type) << " " << token.lexeme << " ";
  std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, std::string>) std::cout << arg;        
  }, token.literal);
  return os;
}

#endif
