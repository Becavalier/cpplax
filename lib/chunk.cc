#include "./chunk.h"
#include "./helper.h"

void ChunkDebugger::simpleInstruction(
  const char* name, 
  typeVMCodeArray::const_iterator& offset) {
  std::cout << name << std::endl;
  offset += 1;
}
void ChunkDebugger::constantInstruction(
  const char* name,
  const Chunk& chunk, 
  typeVMCodeArray::const_iterator& offset) {
    const auto constantIdx = *(offset + 1);
    printf("%-16s index(%4d); const('", name, constantIdx);
    printValue(chunk.constants[constantIdx]);
    printf("')\n");
    offset += 2;
  }
void ChunkDebugger::invokeInstruction(
  const char* name,
  const Chunk& chunk, 
  typeVMCodeArray::const_iterator& offset) {
    const auto constantIdx = *(offset + 1);
    const auto argCount = *(offset + 2);
    printf("%-16s args(%d) index(%4d) '", name, argCount, constantIdx);
    printValue(chunk.constants[constantIdx]);
    printf("\n");
    offset += 3;
  }
void ChunkDebugger::byteInstruction(
  const char* name, 
  const char* unit,
  typeVMCodeArray::const_iterator& offset) {
    auto slot = *(offset + 1);
    printf("%-16s %s(%4d);\n", name, unit, slot);
    offset += 2;
  }
void ChunkDebugger::jumpInstruction(
  const char* name,
  int sign,
  const Chunk& chunk, 
  typeVMCodeArray::const_iterator& offset) {
    auto jump = static_cast<uint16_t>(*(offset + 1) << 8);
    jump |= *(offset + 2);
    const auto rel = offset - chunk.code.cbegin();
    printf("%-16s from(%ld) -> to(%ld)\n", name, rel, rel + 3 + sign * jump);
    offset += 3;
  }
void ChunkDebugger::disassembleInstruction(const Chunk& chunk, typeVMCodeArray::const_iterator& offset) {
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
    case OpCode::OP_CLASS: return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OpCode::OP_GET_PROPERTY: return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OpCode::OP_SET_PROPERTY: return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OpCode::OP_METHOD: return constantInstruction("OP_METHOD", chunk, offset);
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
    case OpCode::OP_CLOSURE: {
      offset++;
      const auto constantIdx = *offset++;
      const auto& constant = chunk.constants[constantIdx];
      printf("%-16s %4d ", "OP_CLOSURE", constantIdx);
      printValue(constant);
      printf("\n");
      const auto function = retrieveFuncObj(std::get<Obj*>(constant));
      for (uint32_t i = 0; i < function->upvalueCount; i++) {
        auto isLocal = *offset++;
        auto index = *offset++; 
        const auto addrPos = offset - 2 - chunk.code.cbegin();
        printf("%04ld    |                     %s %d\n", addrPos, isLocal == 1 ? "local" : "upvalue", index);
      }
      offset += 2;
      return;
    }
    case OpCode::OP_GET_UPVALUE: return byteInstruction("OP_GET_UPVALUE", "index", offset);
    case OpCode::OP_SET_UPVALUE: return byteInstruction("OP_SET_UPVALUE", "index", offset);
    case OpCode::OP_CLOSE_UPVALUE: return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OpCode::OP_INVOKE: return invokeInstruction("OP_INVOKE", chunk, offset);
    default: {
      std::cout << "Unknow opcode: " << +instruction << '.' << std::endl;
      offset += 1;
    }
  }
}
void ChunkDebugger::disassembleChunk(const Chunk& chunk, const char* name) {
  std::cout << "== " << name << " ==\n";
  const auto& code = chunk.code;
  auto offset = code.cbegin();
  while (offset != code.cend()) {
    disassembleInstruction(chunk, offset);
  }
  std::cout << std::endl;
}
