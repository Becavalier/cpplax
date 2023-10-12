#include "./chunk.h"
#include "./helper.h"

void ChunkDebugger::printValue(const typeRuntimeValue& v) {
  std::cout << stringifyVariantValue(v);
}
auto ChunkDebugger::simpleInstruction(const char* name, const typeVMCodeArray::const_iterator& offset) {
  std::cout << name << std::endl;
  return offset + 1;
}
auto ChunkDebugger::constantInstruction(
  const char* name, 
  const Chunk& chunk, 
  const typeVMCodeArray::const_iterator& offset) {
    auto constantIdx = *(offset + 1);
    printf("%-16s index(%4d); const('", name, constantIdx);
    printValue(chunk.constants[constantIdx]);
    printf("')\n");
    return offset + 2;
  }
auto ChunkDebugger::byteInstruction(
  const char* name, 
  const char* unit,
  const typeVMCodeArray::const_iterator& offset) {
    auto slot = *(offset + 1);
    printf("%-16s %s(%4d);\n", name, unit, slot);
    return offset + 2;
  }
auto ChunkDebugger::jumpInstruction(
  const char* name,
  int sign,
  const Chunk& chunk, 
  const typeVMCodeArray::const_iterator& offset) {
    auto jump = static_cast<uint16_t>(*(offset + 1) << 8);
    jump |= *(offset + 2);
    const auto rel = offset - chunk.code.cbegin();
    printf("%-16s from(%ld) -> to(%ld)\n", name, rel, rel + 3 + sign * jump);
    return offset + 3;
  }
typeVMCodeArray::const_iterator 
ChunkDebugger::disassembleInstruction(const Chunk& chunk, const typeVMCodeArray::const_iterator& offset) {
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
    case OpCode::OP_SET_LOCAL: return byteInstruction("OP_SET_LOCAL", "index", offset);
    case OpCode::OP_GET_LOCAL: return byteInstruction("OP_GET_LOCAL", "index", offset);
    case OpCode::OP_CALL: return byteInstruction("OP_CALL", "argno", offset);
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
    case OpCode::OP_JUMP: return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OpCode::OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OpCode::OP_LOOP: return jumpInstruction("OP_LOOP", -1, chunk, offset);
    default: {
      std::cout << "Unknow opcode: " << +instruction << '.' << std::endl;
      return offset + 1;
    }
  }
}
void ChunkDebugger::disassembleChunk(const Chunk& chunk, const char* name) {
  std::cout << "== " << name << " ==\n";
  const auto& code = chunk.code;
  auto offset = code.cbegin();
  while (offset != code.cend()) {
    offset = disassembleInstruction(chunk, offset);
  }
  std::cout << std::endl;
}
