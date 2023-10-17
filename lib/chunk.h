#ifndef	_CHUNK_H
#define	_CHUNK_H

/**
 * Chunk structure, for holding compiled byte code and other meta information.
*/

#include <cstdint>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <format>
#include <ranges>
#include <unordered_map>
#include <utility>
#include "./type.h"  

struct Debugger;
struct Chunk {
  friend struct Debugger;
  typeVMCodeArray code;  // A heterogeneous storage (saving both opcodes and operands).
  typeRuntimeValueArray constants;
  std::vector<size_t> lines;  // Save line information with run-length encoding.
  Chunk() = default;
  void addCode(const std::vector<std::pair<OpCodeType, size_t>>& snapshot) {
    for (auto it = snapshot.cbegin(); it != snapshot.cend(); ++it) {
      addCode(it->first, it->second);
    }
  }
  auto count(void) {
    return code.size();
  }
  void addCode(OpCodeType byte, size_t line) {
    try {
      if (lines.size() == 0 || lines.back() != line) {
        lines.insert(lines.end(), { code.size() == 0 ? 0 : code.size(), line });
      } else {
        *(lines.end() - 2) += 1;  // Increase the upper bound of the length unit.
      }
      code.push_back(byte);
    } catch (const std::bad_alloc& e) {
      std::exit(EXIT_FAILURE);
    }
  }
  size_t getLine(const typeVMCodeArray::const_iterator& codeIt) const {
    const auto codeIdx = static_cast<size_t>(codeIt - code.cbegin());
    return getLine(codeIdx);
  }
  size_t getLine(const size_t codeIdx) const {
    for (auto it = lines.cbegin(); it != lines.cend(); it += 2) {
      if (codeIdx <= *it) {
        return *(it + 1);
      }
    }
    return 0;
  }
  size_t addConstant(const typeRuntimeValue& v) {
    try {
      constants.emplace_back(v);
    } catch (const std::bad_alloc& e) {
      std::exit(EXIT_FAILURE);
    }
    return constants.size() - 1;  // Return the index to the appended value.
  }
  void free(void) {
    code.clear();
    constants.clear();
  }
};

struct ChunkDebugger {
  static void simpleInstruction(const char*, typeVMCodeArray::const_iterator&);
  static void constantInstruction(const char*, const Chunk&, typeVMCodeArray::const_iterator&);
  static void byteInstruction(const char*, const char*, typeVMCodeArray::const_iterator&);
  static void jumpInstruction(const char*, int, const Chunk&, typeVMCodeArray::const_iterator&);
  static void disassembleInstruction(const Chunk&, typeVMCodeArray::const_iterator&);
  static void disassembleChunk(const Chunk&, const char*);
};

#endif 
