# cpplox
The C++ implementation for language Lox.

Lox is a tiny scripting language described in [Bob Nystrom](https://stuffwithstuff.com/)'s book [Crafting Interpreters](https://craftinginterpreters.com/). I've already written an tree-walk interpreter using TypesScript, called [TSLox](https://github.com/zlliang/tslox). This C version is a bytecode virtual machine, developed along with reading [Part III](https://craftinginterpreters.com/a-bytecode-virtual-machine.html) of the book.

### How to use?

```
cmake . -Bbuild
cd ./build
make
```

