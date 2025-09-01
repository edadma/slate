#ifndef SLATE_VM_H
#define SLATE_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Include dynamic_string.h for proper string handling
#include "dynamic_string.h"

// Include dynamic library headers directly
#include "dynamic_array.h"
#include "dynamic_int.h"
#include "dynamic_object.h"
#include "dynamic_buffer.h"

// Include value system
#include "value.h"

// Forward declarations
typedef struct slate_vm slate_vm;

// Debug location for values (NULL when debugging disabled)
typedef struct debug_location {
    int line;
    int column;
    const char* source_text; // Pointer to original source line (not owned)
} debug_location;

// VM instruction opcodes
typedef enum {
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
    OP_NULL_COALESCE, // Pop b, pop a, push a ?? b (a if not null/undefined, else b)
    OP_IN, // Pop object, pop property, push property in object
    OP_INSTANCEOF, // Pop type, pop value, push value instanceof type

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

    // Range operations
    OP_BUILD_RANGE, // Pop end, pop start, build range (operand = exclusive flag)

    // Function operations
    OP_CLOSURE, // Create closure (operand = function index)
    OP_CALL, // Call function (operand = arg count)
    OP_CALL_METHOD, // Call method with implicit receiver (operand = arg count)
    OP_RETURN, // Return from function

    // Control flow
    OP_JUMP, // Unconditional jump (operand = offset)
    OP_JUMP_IF_FALSE, // Jump if top of stack is false (operand = offset)
    OP_JUMP_IF_TRUE, // Jump if top of stack is true (operand = offset)
    OP_LOOP, // Loop back (operand = offset)
    
    // Scope operations
    OP_POP_N, // Pop N values from stack and release them (operand = count)
    OP_POP_N_PRESERVE_TOP, // Pop N values but preserve top value (operand = count)

    // Debug operations
    OP_SET_DEBUG_LOCATION, // Set current debug location (operand = constant index to debug info)
    OP_CLEAR_DEBUG_LOCATION, // Clear current debug location

    // Program flow
    OP_HALT // Stop execution
} opcode;





// Function structure
typedef struct function {
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
typedef struct closure {
    function_t* function;
    value_t* upvalues; // Captured variables from outer scopes
    size_t upvalue_count;
} closure_t;

// Call frame for function calls
typedef struct call_frame {
    closure_t* closure; // Function being executed
    uint8_t* ip; // Instruction pointer
    value_t* slots; // Pointer to function's stack window
} call_frame;

// VM execution state
typedef struct slate_vm {
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

    // Function table - stores all defined functions with proper reference counting
    da_array functions; // Global function table

    // Result register - holds the value of the last executed statement
    value_t result;

    // Debug info for current operation (NULL when debugging disabled)
    debug_location* current_debug;

    // Command line arguments
    char** argv;
    int argc;

    // Memory management
    size_t bytes_allocated; // For GC later
} slate_vm;

// Instruction structure for encoding
typedef struct instruction {
    opcode op;
    uint16_t operand; // 16-bit operand (enough for most uses)
} instruction;

// VM lifecycle functions
slate_vm* vm_create(void);
slate_vm* vm_create_with_args(int argc, char** argv);
void vm_destroy(slate_vm* vm);
void vm_reset(slate_vm* vm);

// Store global String class (accessed by vm.c for string creation)
extern value_t* global_string_class;

// Store global Array class (accessed by vm.c for array creation)
extern value_t* global_array_class;

// Store global Range class (accessed by vm.c for range creation)
extern value_t* global_range_class;

// Store global Iterator class (accessed by vm.c for iterator creation)
extern value_t* global_iterator_class;

// Store global StringBuilder class (accessed by vm.c for string builder creation)
extern value_t* global_string_builder_class;

// Bytecode execution
typedef enum { VM_OK, VM_COMPILE_ERROR, VM_RUNTIME_ERROR, VM_STACK_OVERFLOW, VM_STACK_UNDERFLOW } vm_result;

vm_result vm_run(slate_vm* vm);
vm_result vm_execute(slate_vm* vm, function_t* function);
vm_result vm_interpret(slate_vm* vm, const char* source);


// Stack operations
void vm_push(slate_vm* vm, value_t value);
value_t vm_pop(slate_vm* vm);
value_t vm_peek(slate_vm* vm, int distance);

// Function calling helper for builtin methods
value_t vm_call_function(slate_vm* vm, value_t callable, int arg_count, value_t* args);

// String conversion helper for opcodes
ds_string value_to_string_representation(slate_vm* vm, value_t value);


// Iterator creation helpers
iterator_t* create_array_iterator(da_array array);
iterator_t* create_range_iterator(value_t start, value_t end, int exclusive);
int iterator_has_next(iterator_t* iter);
value_t iterator_next(iterator_t* iter);

// Iterator reference counting
iterator_t* iterator_retain(iterator_t* iter);
void iterator_release(iterator_t* iter);

// Range reference counting
range_t* range_retain(range_t* range);
void range_release(range_t* range);

// Bound method reference counting
bound_method_t* bound_method_retain(bound_method_t* method);
void bound_method_release(bound_method_t* method);

// Class reference counting
class_t* class_retain(class_t* cls);
void class_release(class_t* cls);

// Date/Time reference counting
local_date_t* local_date_retain(local_date_t* date);
void local_date_release(local_date_t* date);
local_time_t* local_time_retain(local_time_t* time);
void local_time_release(local_time_t* time);
local_datetime_t* local_datetime_retain(local_datetime_t* dt);
void local_datetime_release(local_datetime_t* dt);
zoned_datetime_t* zoned_datetime_retain(zoned_datetime_t* zdt);
void zoned_datetime_release(zoned_datetime_t* zdt);
instant_t* instant_retain(instant_t* instant);
void instant_release(instant_t* instant);
duration_t* duration_retain(duration_t* duration);
void duration_release(duration_t* duration);
period_t* period_retain(period_t* period);
void period_release(period_t* period);


// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text);
debug_location* debug_location_copy(const debug_location* debug);
void debug_location_free(debug_location* debug);

// Value utility functions
int is_falsy(value_t value);
int is_truthy(value_t value);
int is_number(value_t value); // Check if value is numeric (int32, bigint, or number)
int values_equal(value_t a, value_t b);
void print_value(slate_vm* vm, value_t value);
void print_for_builtin(slate_vm* vm, value_t value);
double value_to_double(value_t value); // Convert numeric values to double
bool is_int(value_t value); // Check if value represents an integer
int value_to_int(value_t value); // Convert numeric values to int

// Constant pool management
size_t vm_add_constant(slate_vm* vm, value_t value);
value_t vm_get_constant(slate_vm* vm, size_t index);

// Function table management
size_t vm_add_function(slate_vm* vm, function_t* function);
function_t* vm_get_function(slate_vm* vm, size_t index);

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
void vm_runtime_error_with_debug(slate_vm* vm, const char* message);
void vm_runtime_error_with_values(slate_vm* vm, const char* format, const value_t* a, const value_t* b,
                                  debug_location* location);
const char* value_type_name(value_type type);

#endif // SLATE_VM_H
