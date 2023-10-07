#ifndef	_TYPE_H
#define	_TYPE_H

#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>

using typeRuntimeNumericValue= double;

enum TokenType : uint8_t {
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
  FN, 
  FOR, 
  IF, 
  NIL, 
  OR,
  RETURN, 
  SUPER, 
  THIS, 
  TRUE, 
  VAR, 
  WHILE, 
  SOURCE_EOF,
  TOTAL,
};

/**
 * Interpreter Related Types
*/
enum class FunctionType : uint8_t {
  NONE,
  FUNCTION,
  INITIALIZER,
  METHOD,
};

enum class ClassType : uint8_t {
  NONE,
  CLASS,
  SUBCLASS,
};

struct Invokable;
struct ClassInstance;
struct Token;
using typeKeywordList = std::unordered_map<std::string_view, TokenType>;
using typeScopeRecord = std::unordered_map<std::string_view, bool>;

/**
 * VM & Compiler Related Types
*/
using OpCodeType = uint8_t;
using typeVMCodeArray = std::vector<uint8_t>;
enum OpCode : OpCodeType {
  OP_CONSTANT,  // [OpCode, Constant Index (uint8_t)].
  // OP_CONSTANT_LONG,
  OP_RETURN,  // [OpCode].
  OP_NEGATE, 
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_NOT,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_POP, 
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL
};

enum class VMResult : uint8_t {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

enum class HeapObjType : uint8_t {
  OBJ_STRING,
};

struct HeapStringObj;
struct HeapObj {
  HeapObjType type;
  HeapObj* next;  // Make up an intrusive list.
  explicit HeapObj(HeapObjType type, HeapObj* next) : type(type), next(next) {}
  HeapStringObj* toStringObj(void);
  virtual ~HeapObj() {};
};

struct HeapStringObj : public HeapObj {
  std::string str;
  explicit HeapStringObj(std::string_view str, HeapObj** next) : HeapObj(HeapObjType::OBJ_STRING, *next), str(str) {
    *next = this;
  }
  ~HeapStringObj() {
    str.clear();
  }
};

class InternedConstants  {
  std::unordered_map<std::string_view, HeapObj*> internedConstants;
 public:
  InternedConstants() = default;
  HeapObj* add(std::string_view str, HeapObj** objs) {
    const auto target = internedConstants.find(str);
    if (target != internedConstants.end()) {
      return target->second;  // Reuse the existing interned string obj.
    } else {
      const auto heapStr = new HeapStringObj { str, objs };
      internedConstants[heapStr->str] = heapStr;
      return heapStr;  // Generate a new sting obj on the heap.
    }
  }
};

struct Local {
  const Token* name;
  size_t depth;
  bool initialized;
};


/**
 * Core runtime types
*/
using typeRuntimeValue = 
  std::variant<
    // Common fields.
    std::monostate, 
    std::string_view, 
    typeRuntimeNumericValue, 
    bool,
    // Interpreter fields.
    std::shared_ptr<Invokable>, 
    std::shared_ptr<ClassInstance>,
    std::string,
    // VM & Compiler fieldds.
    HeapObj*  // Pointer to the heap value.
  >;
using typeRuntimeValueArray = std::vector<typeRuntimeValue>;

#endif
