#include <iostream>
#include <cstdlib>
#include <sysexits.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <variant>
#include "lib/error.h"
#include "lib/token.h"
#include "lib/scanner.h"
#include "lib/parser.h"
#include "lib/expr.h"
#include "lib/interpreter.h"

#define PATH_ARG_IDX 1

struct Lox {
  static std::shared_ptr<Interpreter> interpreter;
  static void run(const std::string& code) {
    // Scanning (lexing).
    Scanner scanner { code };
    const std::vector<Token> tokens = scanner.scanTokens();

    // Parsing.
    Parser parser { tokens };
    const auto ast = parser.parse();

    // ExprPrinter exprPrinter;
    // if (ast != nullptr) exprPrinter.print(ast);
    // std::cout << '\n';

    if (Error::hadError) return;  

    // Interpret.
    interpreter->interpret(ast);
  }

  static void runFile(const char* path) {
    std::ifstream file;
    file.open(path);
    if (file.good()) {
      std::ostringstream oss;
      oss << file.rdbuf();
      run(oss.str());
      if (Error::hadError) std::exit(EX_DATAERR);
      if (Error::hadRuntimeError) std::exit(EX_SOFTWARE);
    }
  }

  static void runPrompt(void) {
    while (true) {
      std::cout << "\n> ";
      std::string input;
      std::getline(std::cin, input);
      if (input.empty()) break;
      run(input);
      Error::hadError = false;
    }
  }
};

auto Lox::interpreter = std::make_shared<Interpreter>();

int main(int argc, char* argv[]) {
  if (argc > 2) {
    std::cout << "Usage: cpplox [script]" << std::endl;
    std::exit(EX_USAGE);
  } else if (argc == 2) {
    Lox::runFile(argv[PATH_ARG_IDX]);
  } else {
    Lox::runPrompt();
  }
  return 0;
}
