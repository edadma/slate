# Bitty Programming Language

A toy programming language implementation with a complete compilation pipeline from source code to bytecode execution on a stack-based virtual machine.

## Features

- Complete compilation pipeline: Lexer → Parser → AST → Code Generator → Virtual Machine
- Stack-based VM with proper operator precedence
- String concatenation with mixed types (`"Aug " + 23` → `"Aug 23"`)
- Interactive REPL
- Built-in test suite

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