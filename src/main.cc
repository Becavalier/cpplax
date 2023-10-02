#include <iostream>
#include <cstdlib>
#include <sysexits.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <variant>
#include <filesystem>
#include <algorithm>
#include <string_view>
#include "../lib/error.h"
#include "../lib/token.h"
#include "../lib/scanner.h"
#include "../lib/parser.h"
#include "../lib/expr.h"
#include "../lib/interpreter.h"
#include "../lib/resolver.h"
#include "../lib/chunk.h"
#include "../lib/vm.h"
#include "../lib/compiler.h"

#define PATH_ARG_IDX 0

namespace fs = std::filesystem;

static bool useInterpreterMode = true;
static void reportIllegalUsage(void) {
  std::cerr << "Usage: cpplax [-i|-c] [file]" << std::endl;
  std::exit(EX_USAGE);
}
struct Lax {
  static void run(const std::string& code) {
    Scanner scanner { code };  // Scanning (lexing).
    const std::vector<Token> tokens = scanner.scanTokens();
    if (Error::hadError) return;

    if (useInterpreterMode) {
      std::cout << "- Interpreter Mode -\n\n";
      Parser parser { tokens };  // Parsing.
      const auto ast = parser.parse();
      if (Error::hadError) return;  

      Interpreter interpreter;
      Resolver resolver { interpreter };  // Semantic analysis.
      resolver.resolve(ast);
      if (Error::hadError) return;

      interpreter.interpret(ast);  // Interpret.
    } else {
      std::cout << "- Compiler Mode -\n";
      Compiler compiler { tokens };
      const auto chunk = compiler.compile();  // Compile into byte codes.
      if (Error::hadError) return;

      VM vm { chunk };
      vm.interpret();
    }
  }

  static void runFile(const std::string_view path) {
    if (fs::is_regular_file(fs::status(path))) {
      std::ifstream file;
      file.open(path);
      if (file.good()) {
        std::ostringstream oss;
        oss << file.rdbuf();
        run(oss.str());
        if (Error::hadError) std::exit(EX_DATAERR);
        if (Error::hadInterpreterError) std::exit(EX_SOFTWARE);
        return;
      }
    }
    std::cerr << "Error: at '" << path << "', invalid input file." << std::endl;
    std::exit(EX_NOINPUT);
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

int main(int argc, const char* argv[]) {
  if (argc > 3) {
    reportIllegalUsage();
  } 
  std::vector<std::string_view> args(argv + 1, argv + argc);

  // Process input arguments.
  auto iflag = std::find(args.begin(), args.end(), "-i");
  if (iflag != args.end()) {
    args.erase(iflag);    
  } else {
    useInterpreterMode = false;
  }
  auto cflag = std::find(args.begin(), args.end(), "-c");
  if (cflag != args.end()) {
    useInterpreterMode = false;  // Compiler mode goes first.
    args.erase(cflag);
  }
  if (args.size() > 1) {
    reportIllegalUsage();
  } else if (args.size() == 1) {
    Lax::runFile(args.at(PATH_ARG_IDX));
  } else {
    Lax::runPrompt();
  }
  return 0;
}
