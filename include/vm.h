#ifndef BITTY_VM_H
#define BITTY_VM_H

#include <stddef.h>
#include <stdint.h>

// Include dynamic_string.h for proper string handling
#include "/home/ed/CLionProjects/dynamic_string.h/dynamic_string.h"

// Include dynamic library headers directly
#include "/home/ed/CLionProjects/dynamic_array.h/dynamic_array.h"
#include "/home/ed/CLionProjects/dynamic_object.h/dynamic_object.h"
#include "/home/ed/CLionProjects/dynamic_bigint.h/dynamic_bigint.h"

// Forward declarations
typedef struct value value_t;

// Debug location for values (NULL when debugging disabled)
typedef struct debug_location {
    int line;
    int column;
    const char* source_text;  // Pointer to original source line (not owned)
} debug_location;

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
    OP_SET_RESULT, // Pop value from stack and store in result register

    // Arithmetic operations
    OP_ADD, // Pop b, pop a, push a + b
    OP_SUBTRACT, // Pop b, pop a, push a - b
    OP_MULTIPLY, // Pop b, pop a, push a * b
    OP_DIVIDE, // Pop b, pop a, push a / b
    OP_MOD, // Pop b, pop a, push a % b
    OP_POWER, // Pop b, pop a, push a ** b
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

    // Bitwise operations
    OP_BITWISE_AND, // Pop b, pop a, push a & b
    OP_BITWISE_OR, // Pop b, pop a, push a | b
    OP_BITWISE_XOR, // Pop b, pop a, push a ^ b
    OP_BITWISE_NOT, // Pop a, push ~a
    OP_LEFT_SHIFT, // Pop b, pop a, push a << b
    OP_RIGHT_SHIFT, // Pop b, pop a, push a >> b (arithmetic)
    OP_LOGICAL_RIGHT_SHIFT, // Pop b, pop a, push a >>> b (logical)
    OP_FLOOR_DIV, // Pop b, pop a, push floor(a / b)
    OP_INCREMENT, // Pop a, push a + 1
    OP_DECREMENT, // Pop a, push a - 1

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

    // Debug operations
    OP_SET_DEBUG_LOCATION, // Set current debug location (operand = constant index to debug info)
    OP_CLEAR_DEBUG_LOCATION, // Clear current debug location

    // Program flow
    OP_HALT // Stop execution
} opcode;

// VM value types
typedef enum
{
    VAL_NULL,
    VAL_UNDEFINED,
    VAL_BOOLEAN,
    VAL_INT32,     // 32-bit integer (MCU-friendly default)
    VAL_BIGINT,    // Arbitrary precision integer
    VAL_NUMBER,    // Floating point (double)
    VAL_STRING,
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_FUNCTION,
    VAL_CLOSURE,
    VAL_BUILTIN
} value_type;

// VM value structure
typedef struct value
{
    value_type type;
    union
    {
        int boolean;
        int32_t int32;    // 32-bit integer (direct storage)
        db_bigint bigint; // Arbitrary precision integer (ref-counted)
        double number;    // Floating point
        ds_string string; // Using dynamic_string.h!
        da_array array; // Using dynamic_array.h!
        do_object object; // Using dynamic_object.h!
        struct function* function;
        struct closure* closure;
        void* builtin; // Built-in function pointer (cast from builtin_func_t)
    } as;
    debug_location* debug; // Debug info for error reporting (NULL when disabled)
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
typedef struct bitty_vm
{
    // Bytecode execution
    uint8_t* bytecode; // Current bytecode being executed
    uint8_t* ip; // Instruction pointer
    uint8_t* current_instruction; // Start of currently executing instruction (for error reporting)

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
    
    // Result register - holds the value of the last executed statement
    value_t result;
    
    // Debug info for current operation (NULL when debugging disabled)
    debug_location* current_debug;

    // Memory management
    size_t bytes_allocated; // For GC later
} bitty_vm;

// Instruction structure for encoding
typedef struct instruction
{
    opcode op;
    uint16_t operand; // 16-bit operand (enough for most uses)
} instruction;

// VM lifecycle functions
bitty_vm* vm_create(void);
void vm_destroy(bitty_vm* vm);
void vm_reset(bitty_vm* vm);

// Bytecode execution
typedef enum
{
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
    VM_STACK_OVERFLOW,
    VM_STACK_UNDERFLOW
} vm_result;

vm_result vm_execute(bitty_vm* vm, function_t* function);
vm_result vm_interpret(bitty_vm* vm, const char* source);

// Value memory management
value_t vm_retain(value_t value);
void vm_release(value_t value);

// Stack operations
void vm_push(bitty_vm* vm, value_t value);
value_t vm_pop(bitty_vm* vm);
value_t vm_peek(bitty_vm* vm, int distance);

// Value creation functions
value_t make_null(void);
value_t make_undefined(void);
value_t make_boolean(int value);
value_t make_int32(int32_t value);
value_t make_bigint(db_bigint big);
value_t make_number(double value);
value_t make_string(const char* value);
value_t make_array(da_array array);
value_t make_object(do_object object);
value_t make_function(function_t* function);
value_t make_closure(closure_t* closure);
value_t make_builtin(void* builtin_func);

// Value creation functions with debug info
value_t make_null_with_debug(debug_location* debug);
value_t make_undefined_with_debug(debug_location* debug);
value_t make_boolean_with_debug(int value, debug_location* debug);
value_t make_int32_with_debug(int32_t value, debug_location* debug);
value_t make_bigint_with_debug(db_bigint big, debug_location* debug);
value_t make_number_with_debug(double value, debug_location* debug);
value_t make_string_with_debug(const char* value, debug_location* debug);
value_t make_string_ds_with_debug(ds_string str, debug_location* debug);
value_t make_array_with_debug(da_array array, debug_location* debug);
value_t make_object_with_debug(do_object object, debug_location* debug);
value_t make_function_with_debug(function_t* function, debug_location* debug);
value_t make_closure_with_debug(closure_t* closure, debug_location* debug);
value_t make_builtin_with_debug(void* builtin_func, debug_location* debug);

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text);
debug_location* debug_location_copy(const debug_location* debug);
void debug_location_free(debug_location* debug);

// Value utility functions
int is_falsy(value_t value);
int values_equal(value_t a, value_t b);
void print_value(value_t value);
void free_value(value_t value);

// Constant pool management
size_t vm_add_constant(bitty_vm* vm, value_t value);
value_t vm_get_constant(bitty_vm* vm, size_t index);

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
void vm_runtime_error_with_debug(bitty_vm* vm, const char* message);
void vm_runtime_error_with_values(bitty_vm* vm, const char* format, const value_t* a, const value_t* b, debug_location* location);
const char* value_type_name(value_type type);

#endif // BITTY_VM_H
