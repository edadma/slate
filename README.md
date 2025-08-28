# Slate Programming Language

A complete toy programming language implementation with a full compilation pipeline from source code to bytecode execution on a stack-based virtual machine. Designed as a learning project for compiler construction, AST building, and virtual machine implementation.

## Features

### Core Language
- **Complete compilation pipeline**: Lexer → Parser → AST → Code Generator → Virtual Machine
- **Stack-based VM** with proper operator precedence and memory management
- **Multiple numeric types**: 32-bit integers, arbitrary precision BigInt, floating-point
- **Rich arithmetic operations**: `+`, `-`, `*`, `/`, `%`, `//` (floor division), `**` (power)
- **Compound assignments**: `+=`, `-=`, `*=`, `/=`, `%=`, `**=`
- **Bitwise operations**: `&`, `|`, `^`, `~`, `<<`, `>>` (arithmetic), `>>>` (logical)
- **Increment/decrement**: `++x`, `--x` (requires l-values)
- **Variables**: Declaration (`var x = 42`) and assignment (`x = 10`)

### Data Types
- **Numbers**: Integers, BigInt (overflow handling), floating-point
- **Strings**: With escape sequences and concatenation (`"Hello " + 42` → `"Hello 42"`)
- **Booleans**: `true`, `false` with both symbolic (`&&`, `||`, `!`) and keyword (`and`, `or`, `not`) operators
- **Arrays**: Dynamic with concatenation (`[1, 2] + [3, 4]` → `[1, 2, 3, 4]`)
- **Objects**: Hash tables with property access
- **Ranges**: `1..10` (inclusive), `1..<10` (exclusive)
- **Buffers**: Binary data handling with I/O capabilities
- **Special values**: `null`, `undefined`

### Control Flow
- **Conditional expressions**: `if condition then expr else expr`
- **Indentation-based blocks**: Clean syntax without braces
- **While loops**: With optional `do` keyword
- **Infinite loops**: `loop` with `break`/`continue` statements
- **Mixed syntax support**: Single-line and multi-line forms

### Built-in Functions
- **Math**: `abs()`, `sqrt()`, `floor()`, `ceil()`, `round()`, `min()`, `max()`, `random()`
- **Advanced math**: `exp()`, `ln()`, `pow()`, `sign()`
- **Trigonometry**: `sin()`, `cos()`, `tan()`, `asin()`, `acos()`, `atan()`, `atan2()`
- **Angle conversion**: `degrees()`, `radians()`
- **Type checking**: `type()` returns string representation
- **I/O**: `print()` for output, `input()` for user input
- **String conversion**: `parse_int()`, `parse_number()`
- **Command line**: `args()` returns script arguments

### Advanced Features
- **Enhanced REPL**: Multi-line input, line editing, history, word navigation
- **Expression statements**: Automatic result display
- **Error handling**: Precise error locations with caret pointing
- **Debug support**: Comprehensive debugging and bytecode disassembly
- **Memory safety**: Reference counting with proper cleanup
- **Comprehensive test suite**: 214+ tests with 0 failures

## Build

```bash
# Configure and build
cmake --build cmake-build-debug

# Clean build (if needed)
rm -rf cmake-build-debug
```

## Usage

### Command Line Options

```bash
# Interactive REPL
./cmake-build-debug/slate

# Execute a file with arguments
./cmake-build-debug/slate -f script.slate [args...]

# Execute script content directly
./cmake-build-debug/slate --script "print(2 + 3 * 4)"

# Read from stdin
echo "42 + 58" | ./cmake-build-debug/slate --stdin

# Run built-in tests
./cmake-build-debug/slate --test

# Enable debug output
./cmake-build-debug/slate --debug --script "exp(ln(5))"
```

### Example Usage

```bash
# Math functions
./cmake-build-debug/slate --script "sin(radians(30))"    # Result: 0.5
./cmake-build-debug/slate --script "degrees(asin(0.5))"  # Result: 30
./cmake-build-debug/slate --script "sign(-42)"           # Result: -1

# File execution with arguments
./cmake-build-debug/slate -f examples/greeting.slate Alice 25

# Interactive REPL with enhanced editing
./cmake-build-debug/slate
> var x = exp(2)
> print("e^2 = " + x)
e^2 = 7.38906
```

## Running Tests

```bash
# Run comprehensive test suite (214+ tests)
./cmake-build-debug/slate_tests

# Run language built-in tests
./cmake-build-debug/slate --test
```

## Project Structure

```
slate/
├── include/           # Header files
├── src/              # Source implementation
├── tests/            # Unit tests (Unity framework) 
├── examples/         # Example .slate programs
├── cmake-build-debug/ # Build output
└── run               # Convenience script
```

## Examples

Slate supports a rich set of mathematical and programming constructs:

### Math Functions
```slate
print(sqrt(16))           # 4
print(exp(ln(5)))         # 5 
print(sin(radians(90)))   # 1
print(degrees(atan2(1,1))) # 45
```

### Control Flow
```slate
var x = 10
if x > 5 then
    print("x is large")
else 
    print("x is small")
```

### Arrays and Objects
```slate
var arr = [1, 2, 3]
var obj = {name: "Slate", version: 1.0}
print(arr + [4, 5])       # [1, 2, 3, 4, 5]
```