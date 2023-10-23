# cpplax
The C++ implementation for the language Lax which is a dynamically typed language that is similar to JavaScript.
 
### Getting Started

#### Build and Run

Please make sure CMake (at least version 3.20) is well installed on your OS.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

#### Test

Run the below commands after the previous step. By default, the test will be running via the compiler and VM, in order to test the interpreter, please re-run the preceding CMake setup command and specify the environment variable `TEST_TARGET=INTERPRETER`.

```
ctest --test-dir ./build [-V]
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
