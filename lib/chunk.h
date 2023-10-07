#ifndef	_CHUNK_H
#define	_CHUNK_H

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
#include "./helper.h"

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
  size_t addConstant(typeRuntimeValue v) {
    try {
      constants.push_back(v);
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
  static void printValue(const typeRuntimeValue& v) {
    std::cout << stringifyVariantValue(v);
  }
  static auto simpleInstruction(const char* name, const typeVMCodeArray::const_iterator& offset) {
    std::cout << name << std::endl;
    return offset + 1;
  }
  static auto constantInstruction(const char* name, const Chunk& chunk, const typeVMCodeArray::const_iterator& offset) {
    auto constantIdx = *(offset + 1);
    printf("%-16s %4d '", name, constantIdx);
    printValue(chunk.constants[constantIdx]);
    printf("'\n");
    return offset + 2;
  }
  static auto byteInstruction(const char* name, const Chunk& chunk, const typeVMCodeArray::const_iterator& offset) {
    auto slot = *(offset + 1);
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
  }
  static auto disassembleInstruction(const Chunk& chunk, const typeVMCodeArray::const_iterator& offset) {
    const auto offsetPos = offset - chunk.code.cbegin();
    printf("%04ld ", offsetPos);  // Print the offset location.
    if (offsetPos > 0 && chunk.getLine(offsetPos) == chunk.getLine(offsetPos - 1)) {
      printf("   | ");
    } else {
      printf("%4zu ", chunk.getLine(offsetPos));  // Print line information.
    }
    const auto instruction = *offset;
    switch (instruction) {  // Instructions have different sizes. 
      case OpCode::OP_CONSTANT: return constantInstruction("OP_CONSTANT", chunk, offset);
      case OpCode::OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
      case OpCode::OP_GET_GLOBAL: return constantInstruction("OP_GET_GLOBAL", chunk, offset);
      case OpCode::OP_SET_GLOBAL: return constantInstruction("OP_SET_GLOBAL", chunk, offset);
      case OpCode::OP_SET_LOCAL: return byteInstruction("OP_SET_LOCAL", chunk, offset);
      case OpCode::OP_GET_LOCAL: return byteInstruction("OP_GET_LOCAL", chunk, offset);
      case OpCode::OP_NIL: return simpleInstruction("OP_NIL", offset);
      case OpCode::OP_TRUE: return simpleInstruction("OP_TRUE", offset);
      case OpCode::OP_FALSE: return simpleInstruction("OP_FALSE", offset);
      case OpCode::OP_ADD: return simpleInstruction("OP_ADD", offset);
      case OpCode::OP_SUBTRACT: return simpleInstruction("OP_SUBTRACT", offset);
      case OpCode::OP_MULTIPLY: return simpleInstruction("OP_MULTIPLY", offset);
      case OpCode::OP_DIVIDE: return simpleInstruction("OP_DIVIDE", offset);
      case OpCode::OP_NEGATE:  return simpleInstruction("OP_NEGATE", offset);
      case OpCode::OP_RETURN: return simpleInstruction("OP_RETURN", offset);
      case OpCode::OP_NOT: return simpleInstruction("OP_NOT", offset);
      case OpCode::OP_EQUAL: return simpleInstruction("OP_EQUAL", offset);
      case OpCode::OP_GREATER: return simpleInstruction("OP_GREATER", offset);
      case OpCode::OP_LESS: return simpleInstruction("OP_LESS", offset);
      case OpCode::OP_POP: return simpleInstruction("OP_POP", offset);
      default: {
        std::cout << "Unknow opcode: " << +instruction << '.' << std::endl;
        return offset + 1;
      }
    }
  }
  static void disassembleChunk(const Chunk& chunk, const char* name) {
    std::cout << "== " << name << " ==\n";
    const auto& code = chunk.code;
    auto offset = code.cbegin();
    while (offset != code.cend()) {
      offset = disassembleInstruction(chunk, offset);
    }
    std::cout << std::endl;
  }
};

#endif
