# Bitty Programming Language - Design Document

A tiny, functional programming language with closures, prototype-based objects, and JavaScript-like syntax. Built as an educational project to demonstrate AST construction, syntax-directed translation, and VM-based execution.

## Table of Contents

- [Language Overview](#language-overview)
- [Syntax Design](#syntax-design)
- [Grammar](#grammar)
- [AST Design](#ast-design)
- [VM Architecture](#vm-architecture)
- [Closure Implementation](#closure-implementation)
- [Code Generation](#code-generation)
- [Examples](#examples)
- [Implementation Plan](#implementation-plan)

## Language Overview

**Bitty** is designed to demonstrate modern language implementation concepts:

- **Functional features**: Closures, implicit returns, arrow functions
- **Object-oriented features**: Prototype-based objects with dynamic properties
- **Control flow**: Conditionals, loops, early returns
- **Memory management**: Automatic via reference counting (using dynamic_object.h)
- **Execution model**: Stack-based virtual machine with bytecode compilation

### Core Philosophy

- **Minimal but complete** - Small enough to understand fully, complete enough to be useful
- **Functional first** - Functions are first-class values, closures everywhere
- **Dynamic objects** - JavaScript-like object model with prototype inheritance
- **Educational focus** - Clear implementation demonstrating compiler techniques

## Syntax Design

### Functions

Functions use arrow syntax with implicit returns:

```javascript
// Expression functions (single expression)
var add = (x, y) -> x + y;
var square = (x) -> x * x;

// Block functions (multiple statements, last is return value)
var factorial = (n) -> {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
};

// Implicit return from last statement
var compute = (x) -> {
    var temp = x * 2;
    var result = temp + 1;
    result  // This value is returned
};
```

### Objects

Prototype-based objects with dynamic properties:

```javascript
// Object literals
var person = {
    name: "Alice",
    age: 25
};

// Method assignment
person.greet = (name) -> print("Hello " + name);

// Prototype inheritance
var student = {
    __proto__: person,
    grade: "A"
};
```

### Arrays

Dynamic arrays with numeric indexing:

```javascript
// Array literals
var numbers = [1, 2, 3, 4, 5];
var mixed = [42, "hello", {name: "test"}];

// Array access and modification
var first = numbers[0];         // 1
numbers[1] = 99;               // [1, 99, 3, 4, 5]
numbers[numbers.length] = 6;   // Append: [1, 99, 3, 4, 5, 6]

// Array methods
numbers.push(7);               // [1, 99, 3, 4, 5, 6, 7]
var last = numbers.pop();      // 7, array: [1, 99, 3, 4, 5, 6]

// Iteration
var i = 0;
while (i < numbers.length) {
    print(numbers[i]);
    i = i + 1;
}
```

### Strings

Immutable string values with concatenation and indexing:

```javascript
// String literals
var greeting = "Hello";
var name = "World";

// String concatenation
var message = greeting + " " + name + "!";  // "Hello World!"

// String indexing (read-only)
var first_char = message[0];     // "H"
var length = message.length;     // 12

// String methods
var upper = message.toUpper();   // "HELLO WORLD!"
var lower = message.toLower();   // "hello world!"
var sub = message.substr(6, 5);  // "World"

// String comparison
if (greeting == "Hello") {
    print("Match!");
}
```

### Undefined Value

Bitty includes a special `undefined` value with unique behavior:

```javascript
// undefined returned by missing operations
var result = someObject.nonExistentProperty;  // Returns undefined

// Comparison with undefined
if (result == undefined) {
    print("Property does not exist");
}

// undefined in conditionals (falsy)
if (undefined) {
    print("This won't execute");  // undefined is falsy
}
```

**Special Properties of `undefined`:**

1. **Available as literal** - Users can write `undefined` in code to test/compare values
2. **Returned by missing operations** - Non-existent object properties return `undefined`
3. **Cannot be assigned** - `var x = undefined` is **illegal** (compile error)
4. **Cannot be stored** - Object properties cannot be set to `undefined` (runtime error)
5. **Falsy value** - `undefined` evaluates to `false` in conditionals

**Legal uses:**
```javascript
// ✅ Comparison/testing
if (obj.prop == undefined) { ... }
var x = (obj.prop == undefined) ? "default" : obj.prop;

// ✅ Temporary expression results
someFunction() == undefined;  // Returns boolean
```

**Illegal uses:**
```javascript
// ❌ Variable assignment
var x = undefined;        // Compile error

// ❌ Property assignment  
obj.prop = undefined;     // Runtime error

// ❌ Function return
return undefined;         // Compile error

// ❌ Function parameters
func(undefined);          // Runtime error

// ❌ Array elements
arr[0] = undefined;       // Runtime error
```

This makes `undefined` a special "sentinel" value indicating absence/invalidity that can be observed but not explicitly stored.

### Variables and Assignment

```javascript
var x = 42;        // Declaration with initialization
x = x + 1;         // Assignment
var y = x;         // Copy value
```

### Control Flow

```javascript
// Conditionals
if (x > 0) {
    print("positive");
} else {
    print("non-positive");
}

// While loops
var i = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}
```

## Grammar

```
program     := statement*

statement   := varDecl | assignment | ifStmt | whileStmt | returnStmt | expression ';'

varDecl     := 'var' IDENTIFIER '=' expression ';'
assignment  := IDENTIFIER '=' expression ';'
ifStmt      := 'if' '(' expression ')' statement ('else' statement)?
whileStmt   := 'while' '(' expression ')' statement
returnStmt  := 'return' expression? ';'

expression  := comparison
comparison  := addition (('==' | '!=' | '<' | '>' | '<=' | '>=') addition)*
addition    := multiplication (('+' | '-') multiplication)*
multiplication := unary (('*' | '/') unary)*
unary       := ('!' | '-') unary | postfix
postfix     := primary ('(' arguments? ')' | '.' IDENTIFIER)*
primary     := NUMBER | STRING | IDENTIFIER | function | object | '(' expression ')'

function    := '(' parameters? ')' '->' (expression | block)
parameters  := IDENTIFIER (',' IDENTIFIER)*
block       := '{' statement* '}'
arguments   := expression (',' expression)*

object      := '{' (property (',' property)*)? '}'
property    := IDENTIFIER ':' expression
```

### Tokens

```
// Literals
NUMBER      := [0-9]+(\.[0-9]+)?
STRING      := '"' [^"]* '"'
IDENTIFIER  := [a-zA-Z_][a-zA-Z0-9_]*

// Keywords
var, if, else, while, return

// Operators
+, -, *, /, ==, !=, <, >, <=, >=, =, !

// Punctuation
(, ), {, }, ,, ;, :, ., ->
```

## AST Design

### Node Types

```c
typedef enum {
    // Literals
    AST_NUMBER,
    AST_STRING, 
    AST_IDENTIFIER,
    
    // Binary operations
    AST_BINARY_OP,
    
    // Statements
    AST_VAR_DECL,
    AST_ASSIGNMENT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_RETURN_STMT,
    AST_BLOCK,
    
    // Expressions
    AST_FUNCTION,
    AST_CALL,
    AST_OBJECT,
    AST_PROPERTY,
    AST_MEMBER_ACCESS,
    
    // Program
    AST_PROGRAM
} ast_node_type;
```

### Node Structures

```c
typedef struct ast_node {
    ast_node_type type;
    int line;  // For error reporting
    
    union {
        // Literals
        double number;
        char* string;
        char* identifier;
        
        // Binary operations
        struct {
            struct ast_node* left;
            struct ast_node* right; 
            char* operator;
        } binary_op;
        
        // Statements
        struct {
            char* name;
            struct ast_node* value;
        } var_decl;
        
        struct {
            char* name;
            struct ast_node* value;
        } assignment;
        
        struct {
            struct ast_node* condition;
            struct ast_node* then_branch;
            struct ast_node* else_branch;  // Optional
        } if_stmt;
        
        struct {
            struct ast_node* condition;
            struct ast_node* body;
        } while_stmt;
        
        struct {
            struct ast_node* value;  // Optional
        } return_stmt;
        
        struct {
            struct ast_node** statements;
            int count;
        } block;
        
        // Functions
        struct {
            char** parameters;
            int param_count;
            struct ast_node* body;  // Expression or block
            char** free_vars;       // For closure analysis
            int free_var_count;
        } function;
        
        struct {
            struct ast_node* callee;
            struct ast_node** arguments;
            int arg_count;
        } call;
        
        // Objects
        struct {
            char** keys;
            struct ast_node** values;
            int property_count;
        } object;
        
        struct {
            struct ast_node* object;
            char* property;
        } member_access;
        
        // Program
        struct {
            struct ast_node** statements;
            int count;
        } program;
    } as;
} ast_node_t;
```

## VM Architecture

### Instruction Set

```c
typedef enum {
    // Stack operations
    OP_LOAD_CONST,      // Push constant onto stack
    OP_POP,             // Remove top of stack
    
    // Variables
    OP_LOAD_VAR,        // Push variable value onto stack
    OP_STORE_VAR,       // Store stack top to variable
    OP_LOAD_CAPTURED,   // Load from closure environment
    OP_STORE_CAPTURED,  // Store to closure environment
    
    // Arithmetic
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    
    // Comparison
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,
    
    // Logical
    OP_NOT,
    
    // Control flow
    OP_JUMP,            // Unconditional jump
    OP_JUMP_IF_FALSE,   // Jump if stack top is false
    OP_JUMP_IF_TRUE,    // Jump if stack top is true
    
    // Functions
    OP_CREATE_CLOSURE,  // Create closure with captured environment
    OP_CALL,            // Call function
    OP_RETURN,          // Return from function
    
    // Objects
    OP_CREATE_OBJECT,   // Create empty object
    OP_SET_PROPERTY,    // obj[key] = value (pops obj, key, value)
    OP_GET_PROPERTY,    // obj[key] (pops obj, key, pushes value)
    
    // Built-ins
    OP_PRINT,           // Print stack top
    OP_HALT             // Stop execution
} vm_opcode_t;
```

### VM State

```c
typedef struct {
    // Execution state
    unsigned char* bytecode;
    int ip;                     // Instruction pointer
    int bytecode_size;
    
    // Stack
    vm_value_t* stack;
    int stack_top;
    int stack_capacity;
    
    // Call frames
    vm_frame_t* frames;
    int frame_count;
    int frame_capacity;
    
    // Constants pool
    vm_value_t* constants;
    int constant_count;
    
    // Globals
    do_object globals;
    
    // Memory management
    do_object* objects;         // All allocated objects for GC
    int object_count;
} vm_t;

typedef struct {
    do_object locals;           // Local variables
    do_object captured;         // Captured variables from closure
    int return_address;         // Where to return to
    int stack_base;             // Stack position when frame was created
} vm_frame_t;
```

### Value System

```c
typedef enum {
    VAL_NUMBER,
    VAL_STRING, 
    VAL_BOOLEAN,
    VAL_NULL,
    VAL_OBJECT,
    VAL_CLOSURE
} vm_value_type_t;

typedef struct {
    vm_value_type_t type;
    union {
        double number;
        char* string;
        bool boolean;
        do_object object;
        struct {
            unsigned char* bytecode;
            int bytecode_size;
            char** parameters;
            int param_count;
            do_object captured_env;
        } closure;
    } as;
} vm_value_t;
```

## Closure Implementation

### Closure Analysis

During AST traversal, identify free variables:

```c
typedef struct {
    char** free_vars;
    int free_var_count;
    char** local_vars;
    int local_var_count;
} scope_analyzer_t;

void analyze_free_variables(ast_node_t* function_node, scope_analyzer_t* parent_scope) {
    scope_analyzer_t local_scope = {0};
    
    // Add parameters to local scope
    for (int i = 0; i < function_node->as.function.param_count; i++) {
        add_local_var(&local_scope, function_node->as.function.parameters[i]);
    }
    
    // Analyze function body
    find_free_variables(function_node->as.function.body, &local_scope, parent_scope);
    
    // Store free variables in function node
    function_node->as.function.free_vars = local_scope.free_vars;
    function_node->as.function.free_var_count = local_scope.free_var_count;
}
```

### Runtime Closure Creation

```c
vm_value_t create_closure(unsigned char* bytecode, int size, char** params, int param_count, 
                         char** free_vars, int free_var_count, vm_frame_t* current_frame) {
    vm_value_t closure_val = {VAL_CLOSURE};
    
    closure_val.as.closure.bytecode = bytecode;
    closure_val.as.closure.bytecode_size = size;
    closure_val.as.closure.parameters = params;
    closure_val.as.closure.param_count = param_count;
    
    // Capture free variables from current environment
    closure_val.as.closure.captured_env = do_create(vm_value_release);
    
    for (int i = 0; i < free_var_count; i++) {
        const char* var_name = free_vars[i];
        
        // Look up variable in current frame
        vm_value_t* value = resolve_variable(var_name, current_frame);
        if (value) {
            DO_SET(closure_val.as.closure.captured_env, var_name, *value);
        }
    }
    
    return closure_val;
}
```

## Code Generation

### Expression Code Generation

```c
void codegen_expression(ast_node_t* expr, bytecode_builder_t* builder) {
    switch (expr->type) {
        case AST_NUMBER:
            emit_instruction(builder, OP_LOAD_CONST);
            emit_constant(builder, make_number(expr->as.number));
            break;
            
        case AST_BINARY_OP:
            codegen_expression(expr->as.binary_op.left, builder);
            codegen_expression(expr->as.binary_op.right, builder);
            emit_binary_op(builder, expr->as.binary_op.operator);
            break;
            
        case AST_IDENTIFIER:
            emit_instruction(builder, OP_LOAD_VAR);
            emit_string(builder, expr->as.identifier);
            break;
            
        case AST_FUNCTION:
            codegen_function(expr, builder);
            break;
            
        case AST_CALL:
            // Generate arguments (right to left for stack)
            for (int i = expr->as.call.arg_count - 1; i >= 0; i--) {
                codegen_expression(expr->as.call.arguments[i], builder);
            }
            codegen_expression(expr->as.call.callee, builder);
            emit_instruction(builder, OP_CALL);
            emit_byte(builder, expr->as.call.arg_count);
            break;
    }
}
```

### Control Flow Code Generation

```c
void codegen_if_statement(ast_node_t* if_stmt, bytecode_builder_t* builder) {
    // Generate condition
    codegen_expression(if_stmt->as.if_stmt.condition, builder);
    
    // Jump to else branch if condition is false
    emit_instruction(builder, OP_JUMP_IF_FALSE);
    int else_jump = emit_jump_placeholder(builder);
    
    // Generate then branch
    codegen_statement(if_stmt->as.if_stmt.then_branch, builder);
    
    if (if_stmt->as.if_stmt.else_branch) {
        // Jump over else branch
        emit_instruction(builder, OP_JUMP);
        int end_jump = emit_jump_placeholder(builder);
        
        // Patch else jump to point here
        patch_jump(builder, else_jump);
        
        // Generate else branch
        codegen_statement(if_stmt->as.if_stmt.else_branch, builder);
        
        // Patch end jump
        patch_jump(builder, end_jump);
    } else {
        // Patch else jump to point to end
        patch_jump(builder, else_jump);
    }
}
```

### Function Code Generation

```c
void codegen_function(ast_node_t* func_node, bytecode_builder_t* builder) {
    // Create separate bytecode for function body
    bytecode_builder_t func_builder = {0};
    
    // Generate function body
    if (func_node->as.function.body->type == AST_BLOCK) {
        // Block function - generate all statements
        ast_node_t* block = func_node->as.function.body;
        for (int i = 0; i < block->as.block.count; i++) {
            codegen_statement(block->as.block.statements[i], &func_builder);
            
            // If this is the last statement and it's an expression, don't pop
            if (i == block->as.block.count - 1) {
                if (is_expression_statement(block->as.block.statements[i])) {
                    // Leave value on stack as return value
                } else {
                    // Push null as default return value
                    emit_instruction(&func_builder, OP_LOAD_CONST);
                    emit_constant(&func_builder, make_null());
                }
            } else {
                // Pop intermediate values
                emit_instruction(&func_builder, OP_POP);
            }
        }
    } else {
        // Expression function - generate expression
        codegen_expression(func_node->as.function.body, &func_builder);
    }
    
    emit_instruction(&func_builder, OP_RETURN);
    
    // Create closure at runtime
    emit_instruction(builder, OP_CREATE_CLOSURE);
    emit_bytecode_chunk(builder, func_builder.bytecode, func_builder.size);
    emit_string_array(builder, func_node->as.function.parameters, func_node->as.function.param_count);
    emit_string_array(builder, func_node->as.function.free_vars, func_node->as.function.free_var_count);
}
```

## Examples

### Closures

```javascript
// Counter closure
var makeCounter = () -> {
    var count = 0;
    return () -> {
        count = count + 1;
        count
    };
};

var counter1 = makeCounter();
var counter2 = makeCounter();

print(counter1()); // 1
print(counter1()); // 2
print(counter2()); // 1 (independent)
```

### Object-Oriented Programming

```javascript
// Constructor function
var Person = (name, age) -> {
    var obj = {
        name: name,
        age: age
    };
    
    obj.greet = () -> print("Hello, I'm " + obj.name);
    obj.birthday = () -> {
        obj.age = obj.age + 1;
        print(obj.name + " is now " + obj.age);
    };
    
    return obj;
};

var alice = Person("Alice", 25);
alice.greet();     // "Hello, I'm Alice"
alice.birthday();  // "Alice is now 26"
```

### Higher-Order Functions

```javascript
var map = (arr, fn) -> {
    var result = {};
    var i = 0;
    while (i < arr.length) {
        result[i] = fn(arr[i]);
        i = i + 1;
    }
    result.length = arr.length;
    return result;
};

var numbers = {0: 1, 1: 2, 2: 3, length: 3};
var doubled = map(numbers, (x) -> x * 2);
```

## Implementation Plan

### Phase 1: Basic Infrastructure
1. **Lexer** - Tokenize source code
2. **Basic AST nodes** - Numbers, identifiers, binary operations
3. **Simple parser** - Parse expressions
4. **VM skeleton** - Stack machine with basic operations
5. **Code generator** - Emit bytecode for simple expressions

### Phase 2: Variables and Functions
1. **Variable declarations** - `var x = expr;`
2. **Assignment** - `x = expr;`
3. **Function parsing** - Arrow functions with parameters
4. **Function calls** - Basic call mechanism
5. **Return statements** - Early returns

### Phase 3: Control Flow
1. **Conditionals** - `if/else` statements
2. **While loops** - `while (condition) statement`
3. **Jump instructions** - `OP_JUMP`, `OP_JUMP_IF_FALSE`
4. **Label management** - Forward/backward jump patching

### Phase 4: Objects
1. **Object literals** - `{key: value}`
2. **Property access** - `obj.prop`, `obj[key]`
3. **Property assignment** - `obj.prop = value`
4. **Dynamic properties** - Runtime property addition

### Phase 5: Closures
1. **Free variable analysis** - Identify captured variables
2. **Closure creation** - `OP_CREATE_CLOSURE` instruction
3. **Environment capture** - Copy variables into closure
4. **Closure calls** - Merge captured + local environments

### Phase 6: Advanced Features
1. **Implicit returns** - Last expression in block
2. **Built-in functions** - `print`, array operations
3. **Error handling** - Runtime error reporting
4. **REPL** - Interactive mode
5. **File execution** - Run `.bitty` files

### Testing Strategy

Each phase includes comprehensive tests:
- **Unit tests** - Individual components (lexer, parser, codegen)
- **Integration tests** - End-to-end compilation and execution
- **Language tests** - `.bitty` programs that should work correctly
- **Error tests** - Invalid programs should fail gracefully

## File Structure

```
bitty/
├── src/
│   ├── main.c           # REPL and file execution
│   ├── lexer.c/.h       # Tokenization
│   ├── parser.c/.h      # AST construction
│   ├── ast.c/.h         # AST node definitions and utilities
│   ├── analyzer.c/.h    # Free variable analysis
│   ├── codegen.c/.h     # Bytecode generation
│   ├── vm.c/.h          # Virtual machine
│   └── value.c/.h       # Value system and memory management
├── tests/
│   ├── unit/            # Unit tests for each component
│   ├── integration/     # End-to-end tests  
│   └── examples/        # Sample .bitty programs
├── examples/
│   ├── hello.bitty
│   ├── fibonacci.bitty
│   ├── closures.bitty
│   └── objects.bitty
└── README.md
```

This design provides a complete roadmap for building **Bitty** - a small but feature-complete functional programming language that demonstrates modern compiler implementation techniques while being educational and fun to build!