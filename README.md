# cpplax
The C++ implementation for the language Lax.

Lax is a language designed based on Lox (a tiny scripting language described in [Bob Nystrom](https://stuffwithstuff.com/)'s book [Crafting Interpreters](https://craftinginterpreters.com/)).
 
### How to use it?

```
cmake . -Bbuild
cd ./build
make
```

### Grammar

```bnf
program        → declaration* EOF ;
declaration    → classDecl
               | funDecl
               | varDecl
               | statement ;
classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )?
                 "{" function* "}" ;
funDecl        → "fun" function ;
varDecl        → "var" IDENTIFIER ( "=" expression )? ";" ;
```

#### Statements

```bnf
statement      → exprStmt
               | forStmt
               | ifStmt
               | returnStmt
               | whileStmt
               | block ;
exprStmt       → expression ";" ;
forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
                           expression? ";"
                           expression? ")" statement ;
ifStmt         → "if" "(" expression ")" statement
                 ( "else" statement )? ;
returnStmt     → "return" expression? ";" ;
whileStmt      → "while" "(" expression ")" statement ;
block          → "{" declaration* "}" ;
```

#### Expressions

```bnf
expression     → assignment ;
assignment     → ( call "." )? IDENTIFIER "=" assignment
               | logic_or ;

logic_or       → logic_and ( "or" logic_and )* ;
logic_and      → equality ( "and" equality )* ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary | call ;
call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
primary        → "true" | "false" | "nil" | "this"
               | NUMBER | STRING | IDENTIFIER | "(" expression ")"
               | "super" "." IDENTIFIER ;
```
#### Helpers

```bnf
function       → IDENTIFIER "(" parameters? ")" block ;
parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
arguments      → expression ( "," expression )* ;
```


### Improvements

- [ ] Look up variables via indices in the corresponding environment.
- [ ] Support "break" and "continue".
- [ ] Support "++" and "--" operators.
- [ ] Support bitwise operators.
- [ ] Support ternary operator "?:".

