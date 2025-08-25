# Bitty Programming Language

A toy programming language implementation with a complete compilation pipeline from source code to bytecode execution on a stack-based virtual machine.

## Features

- Complete compilation pipeline: Lexer → Parser → AST → Code Generator → Virtual Machine
- Stack-based VM with proper operator precedence
- Rich arithmetic operations including power operator (`2 ** 3` → `8`)
- String literals with escape sequences (`\n`, `\t`, `\"`, `\\`)
- String concatenation with mixed types (`"Aug " + 23` → `"Aug 23"`)
- Arrays with concatenation support (`[1, 2] + [3, 4]` → `[1, 2, 3, 4]`)
- Object literals with property access
- Conditional expressions (`if/else`) that work as first-class expressions
- Built-in functions (print, type, math functions)
- Forth-style comments using backslash (`\\ This is a comment`)
- Interactive REPL with multi-line support
- Comprehensive test suite (77 tests)

## Build

```bash
cmake -B build
cmake --build build
```

## Usage

```bash
# Interactive REPL
./build/bitty

# Execute a file
./build/bitty script.bitty

# Run built-in tests
./build/bitty --test

# Enable debug output
./build/bitty --debug
```

## Running Tests

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
./build/bitty_tests
```

## Project Structure

```
bitty/
├── include/        # Header files
├── src/            # Source files
├── tests/          # Unit tests (Unity framework)
└── cmake-build-debug/
```