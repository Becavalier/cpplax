#include <iostream>
#include <cstdlib>
#include <sysexits.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <variant>
#include <filesystem>
#include "../lib/error.h"
#include "../lib/token.h"
#include "../lib/scanner.h"
#include "../lib/parser.h"
#include "../lib/expr.h"
#include "../lib/interpreter.h"
#include "../lib/resolver.h"

#define PATH_ARG_IDX 1

namespace fs = std::filesystem;

struct Lax {
  static Interpreter interpreter;
  static void run(const std::string& code) {
    // Scanning (lexing).
    Scanner scanner { code };
    const std::vector<Token> tokens = scanner.scanTokens();

    // Parsing.
    Parser parser { tokens };
    const auto ast = parser.parse();

    if (Error::hadError) return;  

    // Semantic analysis.
    Resolver resolver { &interpreter };
    resolver.resolve(ast);

    // Interpret.
    interpreter.interpret(ast);
  }

  static void runFile(const char* path) {
    if (fs::is_regular_file(fs::status(path))) {
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

Interpreter Lax::interpreter {};

int main(int argc, char* argv[]) {
  if (argc > 2) {
    std::cout << "Usage: cpplox [script]" << std::endl;
    std::exit(EX_USAGE);
  } else if (argc == 2) {
    Lax::runFile(argv[PATH_ARG_IDX]);
  } else {
    Lax::runPrompt();
  }
  return 0;
}
