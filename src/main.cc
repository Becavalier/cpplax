#include <iostream>
#include <cstdlib>
#include <sysexits.h>
#include <vector>
#include <fstream>
#include <sstream>
#include "lib/error.h"
#include "lib/token.h"
#include "lib/scanner.h"
#include "lib/expr.h"

#define PATH_ARG_IDX 1

void run(const std::string& code) {
  Scanner scanner { code };
  std::vector<Token> tokens = scanner.scanTokens();
  for (auto i = tokens.begin(); i != tokens.end(); ++i) {
    std::cout << *i << std::endl;
  }
  std::cout << '\n';
}

void runFile(const char* path) {
  std::ifstream file;
  file.open(path);
  if (file.good()) {
    std::stringstream ss;
    ss << file.rdbuf();
    run(ss.str());
    if (Error::hadError) std::exit(EX_DATAERR);
  }
}

void runPrompt(void) {
  while (true) {
    std::cout << "> ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) break;
    run(input);
    Error::hadError = false;
  }
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    std::cout << "Usage: jlox [script]" << std::endl;
    std::exit(EX_USAGE);
  } else if (argc == 2) {
    runFile(argv[PATH_ARG_IDX]);
  } else {
    runPrompt();
  }

  Token::typeLiteral lv1 { 123.0 };
  Token::typeLiteral lv2 {45.67};

  BinaryExpr expr { 
    UnaryExpr {
      Token { TokenType::MINUS, "-", std::monostate {}, 1},
      LiteralExpr { lv1 }
    }, 
    Token { TokenType::STAR, "*", std::monostate {}, 1}, 
    GroupingExpr {
      LiteralExpr { lv2 }
    }, 
  };
  AstPrinter ast {};
  std::cout << std::any_cast<std::string const&>(ast.print(expr)) << std::endl;
  return 0;
}
