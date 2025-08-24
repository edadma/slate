#ifndef BIT_VM_H
#define BIT_VM_H

#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct bit_value bit_value;

// VM instruction opcodes
typedef enum {
    // Stack operations
    OP_PUSH_CONSTANT,   // Push constant from constant pool
    OP_PUSH_NULL,       // Push null value
    OP_PUSH_TRUE,       // Push true value
    OP_PUSH_FALSE,      // Push false value
    OP_POP,             // Pop top of stack
    OP_DUP,             // Duplicate top of stack
    
    // Arithmetic operations
    OP_ADD,             // Pop b, pop a, push a + b
    OP_SUBTRACT,        // Pop b, pop a, push a - b  
    OP_MULTIPLY,        // Pop b, pop a, push a * b
    OP_DIVIDE,          // Pop b, pop a, push a / b
    OP_NEGATE,          // Pop a, push -a
    
    // Comparison operations
    OP_EQUAL,           // Pop b, pop a, push a == b
    OP_NOT_EQUAL,       // Pop b, pop a, push a != b
    OP_LESS,            // Pop b, pop a, push a < b
    OP_LESS_EQUAL,      // Pop b, pop a, push a <= b
    OP_GREATER,         // Pop b, pop a, push a > b
    OP_GREATER_EQUAL,   // Pop b, pop a, push a >= b
    
    // Logical operations
    OP_NOT,             // Pop a, push !a
    OP_AND,             // Pop b, pop a, push a && b (short-circuit)
    OP_OR,              // Pop b, pop a, push a || b (short-circuit)
    
    // Variable operations
    OP_GET_LOCAL,       // Push local variable value
    OP_SET_LOCAL,       // Set local variable value (pops value)
    OP_GET_GLOBAL,      // Push global variable value
    OP_SET_GLOBAL,      // Set global variable value (pops value)
    OP_DEFINE_GLOBAL,   // Define global variable (pops value)
    
    // Object/property operations  
    OP_GET_PROPERTY,    // Pop object, push property value
    OP_SET_PROPERTY,    // Pop value, pop object, set property
    OP_GET_INDEX,       // Pop index, pop object, push indexed value
    OP_SET_INDEX,       // Pop value, pop index, pop object, set indexed value
    
    // Array operations
    OP_BUILD_ARRAY,     // Pop n elements, build array (operand = n)
    
    // Object operations
    OP_BUILD_OBJECT,    // Build object from key-value pairs (operand = n pairs)
    
    // Function operations
    OP_CLOSURE,         // Create closure (operand = function index)
    OP_CALL,            // Call function (operand = arg count)
    OP_RETURN,          // Return from function
    
    // Control flow
    OP_JUMP,            // Unconditional jump (operand = offset)
    OP_JUMP_IF_FALSE,   // Jump if top of stack is false (operand = offset)
    OP_JUMP_IF_TRUE,    // Jump if top of stack is true (operand = offset)
    OP_LOOP,            // Loop back (operand = offset)
    
    // Program flow
    OP_HALT             // Stop execution
} opcode;

// VM value types
typedef enum {
    VAL_NULL,
    VAL_BOOLEAN,
    VAL_NUMBER,
    VAL_STRING,
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_FUNCTION,
    VAL_CLOSURE
} value_type;

// VM value structure
typedef struct bit_value {
    value_type type;
    union {
        int boolean;
        double number;
        char* string;           // For now, will use dynamic_string later
        struct bit_array* array;
        struct bit_object* object;
        struct bit_function* function;
        struct bit_closure* closure;
    } as;
} bit_value;

// Array structure
typedef struct bit_array {
    bit_value* elements;
    size_t count;
    size_t capacity;
} bit_array;

// Object property
typedef struct bit_property {
    char* key;
    bit_value value;
} bit_property;

// Object structure (using dynamic_object.h later)
typedef struct bit_object {
    bit_property* properties;
    size_t count;
    size_t capacity;
} bit_object;

// Function structure
typedef struct bit_function {
    uint8_t* bytecode;      // Function bytecode
    size_t bytecode_length;
    bit_value* constants;   // Function constant pool
    size_t constant_count;
    char** parameter_names; // Parameter names
    size_t parameter_count;
    size_t local_count;     // Total local variables (params + locals)
    char* name;             // Function name (for debugging)
} bit_function;

// Closure structure (function + captured variables)
typedef struct bit_closure {
    bit_function* function;
    bit_value* upvalues;    // Captured variables from outer scopes
    size_t upvalue_count;
} bit_closure;

// Call frame for function calls
typedef struct call_frame {
    bit_closure* closure;   // Function being executed
    uint8_t* ip;            // Instruction pointer
    bit_value* slots;       // Pointer to function's stack window
} call_frame;

// VM execution state
typedef struct bit_vm {
    // Bytecode execution
    uint8_t* bytecode;      // Current bytecode being executed
    uint8_t* ip;            // Instruction pointer
    
    // Value stack
    bit_value* stack;       // Value stack
    bit_value* stack_top;   // Pointer to next free stack slot
    size_t stack_capacity;
    
    // Call stack
    call_frame* frames;     // Call frames
    size_t frame_count;
    size_t frame_capacity;
    
    // Constants
    bit_value* constants;   // Global constant pool
    size_t constant_count;
    size_t constant_capacity;
    
    // Global variables
    bit_object* globals;    // Global variable table
    
    // Memory management
    size_t bytes_allocated; // For GC later
} bit_vm;

// Instruction structure for encoding
typedef struct instruction {
    opcode op;
    uint16_t operand;       // 16-bit operand (enough for most uses)
} instruction;

// VM lifecycle functions
bit_vm* vm_create(void);
void vm_destroy(bit_vm* vm);
void vm_reset(bit_vm* vm);

// Bytecode execution
typedef enum {
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
    VM_STACK_OVERFLOW,
    VM_STACK_UNDERFLOW
} vm_result;

vm_result vm_execute(bit_vm* vm, bit_function* function);
vm_result vm_interpret(bit_vm* vm, const char* source);

// Stack operations
void vm_push(bit_vm* vm, bit_value value);
bit_value vm_pop(bit_vm* vm);
bit_value vm_peek(bit_vm* vm, int distance);

// Value creation functions
bit_value make_null(void);
bit_value make_boolean(int value);
bit_value make_number(double value);
bit_value make_string(const char* value);
bit_value make_array(bit_array* array);
bit_value make_object(bit_object* object);
bit_value make_function(bit_function* function);
bit_value make_closure(bit_closure* closure);

// Value utility functions
int is_falsy(bit_value value);
int values_equal(bit_value a, bit_value b);
void print_value(bit_value value);
void free_value(bit_value value);

// Constant pool management
size_t vm_add_constant(bit_vm* vm, bit_value value);
bit_value vm_get_constant(bit_vm* vm, size_t index);

// Object operations
bit_object* object_create(void);
void object_destroy(bit_object* object);
bit_value object_get(bit_object* object, const char* key);
void object_set(bit_object* object, const char* key, bit_value value);

// Array operations
bit_array* array_create(void);
void array_destroy(bit_array* array);
void array_push(bit_array* array, bit_value value);
bit_value array_get(bit_array* array, size_t index);
void array_set(bit_array* array, size_t index, bit_value value);

// Function operations
bit_function* function_create(const char* name);
void function_destroy(bit_function* function);
bit_closure* closure_create(bit_function* function);
void closure_destroy(bit_closure* closure);

// Bytecode utilities
const char* opcode_name(opcode op);

#endif // BIT_VM_H