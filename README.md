# cpplax
The C++ implementation for the language Lax which is a dynamically typed language that is similar to JavaScript.
 
### Getting Started

#### Build and Run

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

#### Running Lax Code

Step 1: save the below code into a text file named "*fib.lax*".

```lax
// fib.lax
fn fib(n) {
  if (n <= 1) return n;
  return fib(n - 2) + fib(n - 1);
}
print(fib(30));  // "832040".
```

Step 2: invoke the compiler and virtual machine via the below command:

```shell
./build/bin/cpplax fib.lax
```
