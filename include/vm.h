#ifndef BIT_VM_H
#define BIT_VM_H

#include <stddef.h>
#include <stdint.h>

// Include dynamic_string.h for proper string handling
#include "/home/ed/CLionProjects/dynamic_string.h/dynamic_string.h"

// Include dynamic library headers directly
#include "/home/ed/CLionProjects/dynamic_array.h/dynamic_array.h"
#include "/home/ed/CLionProjects/dynamic_object.h/dynamic_object.h"

// Forward declarations
typedef struct value value_t;

// VM instruction opcodes
typedef enum
{
    // Stack operations
    OP_PUSH_CONSTANT, // Push constant from constant pool
    OP_PUSH_NULL, // Push null value
    OP_PUSH_UNDEFINED, // Push undefined value
    OP_PUSH_TRUE, // Push true value
    OP_PUSH_FALSE, // Push false value
    OP_POP, // Pop top of stack
    OP_DUP, // Duplicate top of stack

    // Arithmetic operations
    OP_ADD, // Pop b, pop a, push a + b
    OP_SUBTRACT, // Pop b, pop a, push a - b
    OP_MULTIPLY, // Pop b, pop a, push a * b
    OP_DIVIDE, // Pop b, pop a, push a / b
    OP_NEGATE, // Pop a, push -a

    // Comparison operations
    OP_EQUAL, // Pop b, pop a, push a == b
    OP_NOT_EQUAL, // Pop b, pop a, push a != b
    OP_LESS, // Pop b, pop a, push a < b
    OP_LESS_EQUAL, // Pop b, pop a, push a <= b
    OP_GREATER, // Pop b, pop a, push a > b
    OP_GREATER_EQUAL, // Pop b, pop a, push a >= b

    // Logical operations
    OP_NOT, // Pop a, push !a
    OP_AND, // Pop b, pop a, push a && b (short-circuit)
    OP_OR, // Pop b, pop a, push a || b (short-circuit)

    // Variable operations
    OP_GET_LOCAL, // Push local variable value
    OP_SET_LOCAL, // Set local variable value (pops value)
    OP_GET_GLOBAL, // Push global variable value
    OP_SET_GLOBAL, // Set global variable value (pops value)
    OP_DEFINE_GLOBAL, // Define global variable (pops value)

    // Object/property operations
    OP_GET_PROPERTY, // Pop object, push property value
    OP_SET_PROPERTY, // Pop value, pop object, set property
    OP_GET_INDEX, // Pop index, pop object, push indexed value
    OP_SET_INDEX, // Pop value, pop index, pop object, set indexed value

    // Array operations
    OP_BUILD_ARRAY, // Pop n elements, build array (operand = n)

    // Object operations
    OP_BUILD_OBJECT, // Build object from key-value pairs (operand = n pairs)

    // Function operations
    OP_CLOSURE, // Create closure (operand = function index)
    OP_CALL, // Call function (operand = arg count)
    OP_RETURN, // Return from function

    // Control flow
    OP_JUMP, // Unconditional jump (operand = offset)
    OP_JUMP_IF_FALSE, // Jump if top of stack is false (operand = offset)
    OP_JUMP_IF_TRUE, // Jump if top of stack is true (operand = offset)
    OP_LOOP, // Loop back (operand = offset)

    // Program flow
    OP_HALT // Stop execution
} opcode;

// VM value types
typedef enum
{
    VAL_NULL,
    VAL_UNDEFINED,
    VAL_BOOLEAN,
    VAL_NUMBER,
    VAL_STRING,
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_FUNCTION,
    VAL_CLOSURE
} value_type;

// VM value structure
typedef struct value
{
    value_type type;
    union
    {
        int boolean;
        double number;
        ds_string string; // Using dynamic_string.h!
        da_array array; // Using dynamic_array.h!
        do_object object; // Using dynamic_object.h!
        struct function* function;
        struct closure* closure;
    } as;
} value_t;

// Function structure
typedef struct function
{
    uint8_t* bytecode; // Function bytecode
    size_t bytecode_length;
    value_t* constants; // Function constant pool
    size_t constant_count;
    char** parameter_names; // Parameter names
    size_t parameter_count;
    size_t local_count; // Total local variables (params + locals)
    char* name; // Function name (for debugging)
    void* debug; // Optional debug information (debug_info*)
} function_t;

// Closure structure (function + captured variables)
typedef struct closure
{
    function_t* function;
    value_t* upvalues; // Captured variables from outer scopes
    size_t upvalue_count;
} closure_t;

// Call frame for function calls
typedef struct call_frame
{
    closure_t* closure; // Function being executed
    uint8_t* ip; // Instruction pointer
    value_t* slots; // Pointer to function's stack window
} call_frame;

// VM execution state
typedef struct bit_vm
{
    // Bytecode execution
    uint8_t* bytecode; // Current bytecode being executed
    uint8_t* ip; // Instruction pointer

    // Value stack
    value_t* stack; // Value stack
    value_t* stack_top; // Pointer to next free stack slot
    size_t stack_capacity;

    // Call stack
    call_frame* frames; // Call frames
    size_t frame_count;
    size_t frame_capacity;

    // Constants
    value_t* constants; // Global constant pool
    size_t constant_count;
    size_t constant_capacity;

    // Global variables
    do_object globals; // Global variable table

    // Memory management
    size_t bytes_allocated; // For GC later
} bit_vm;

// Instruction structure for encoding
typedef struct instruction
{
    opcode op;
    uint16_t operand; // 16-bit operand (enough for most uses)
} instruction;

// VM lifecycle functions
bit_vm* vm_create(void);
void vm_destroy(bit_vm* vm);
void vm_reset(bit_vm* vm);

// Bytecode execution
typedef enum
{
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
    VM_STACK_OVERFLOW,
    VM_STACK_UNDERFLOW
} vm_result;

vm_result vm_execute(bit_vm* vm, function_t* function);
vm_result vm_interpret(bit_vm* vm, const char* source);

// Stack operations
void vm_push(bit_vm* vm, value_t value);
value_t vm_pop(bit_vm* vm);
value_t vm_peek(bit_vm* vm, int distance);

// Value creation functions
value_t make_null(void);
value_t make_undefined(void);
value_t make_boolean(int value);
value_t make_number(double value);
value_t make_string(const char* value);
value_t make_array(da_array array);
value_t make_object(do_object object);
value_t make_function(function_t* function);
value_t make_closure(closure_t* closure);

// Value utility functions
int is_falsy(value_t value);
int values_equal(value_t a, value_t b);
void print_value(value_t value);
void free_value(value_t value);

// Constant pool management
size_t vm_add_constant(bit_vm* vm, value_t value);
value_t vm_get_constant(bit_vm* vm, size_t index);

// Note: Object and array operations now use dynamic_object.h and dynamic_array.h
// No separate functions needed - we'll use the library APIs directly

// Function operations
function_t* function_create(const char* name);
void function_destroy(function_t* function);
closure_t* closure_create(function_t* function);
void closure_destroy(closure_t* closure);

// Bytecode utilities
const char* opcode_name(opcode op);

// Debug utilities
void* vm_get_debug_info_at(function_t* function, size_t bytecode_offset);
void vm_runtime_error_with_debug(bit_vm* vm, const char* message);

#endif // BIT_VM_H
