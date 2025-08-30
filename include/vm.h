#ifndef SLATE_VM_H
#define SLATE_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Include dynamic_string.h for proper string handling
#include "/home/ed/CLionProjects/dynamic_string.h/dynamic_string.h"

// Include dynamic library headers directly
#include "/home/ed/CLionProjects/dynamic_array.h/dynamic_array.h"
#include "/home/ed/CLionProjects/dynamic_int.h/dynamic_int.h"
#include "/home/ed/CLionProjects/dynamic_object.h/dynamic_object.h"
#define DB_IMPLEMENTATION
#include "/home/ed/CLionProjects/dynamic_buffer.h/dynamic_buffer.h"

// Forward declarations
typedef struct value value_t;
typedef struct slate_vm slate_vm;
typedef value_t (*native_t)(slate_vm* vm, int arg_count, value_t* args);

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

    // Debug operations
    OP_SET_DEBUG_LOCATION, // Set current debug location (operand = constant index to debug info)
    OP_CLEAR_DEBUG_LOCATION, // Clear current debug location

    // Program flow
    OP_HALT // Stop execution
} opcode;

// VM value types
typedef enum {
    VAL_NULL,
    VAL_UNDEFINED,
    VAL_BOOLEAN,
    VAL_INT32, // 32-bit integer (MCU-friendly default)
    VAL_BIGINT, // Arbitrary precision integer
    VAL_NUMBER, // Floating point (double)
    VAL_STRING,
    VAL_STRING_BUILDER, // String builder for constructing strings
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_CLASS, // Class definition (prototype holder)
    VAL_RANGE, // Range object (1..10, 1..<10)
    VAL_ITERATOR, // Iterator object for arrays, ranges, etc.
    VAL_BUFFER, // Byte buffer for binary data
    VAL_BUFFER_BUILDER, // Buffer builder for constructing buffers
    VAL_BUFFER_READER, // Buffer reader for parsing buffers
    VAL_FUNCTION,
    VAL_CLOSURE,
    VAL_NATIVE,
    VAL_BOUND_METHOD,
    // Date/Time types
    VAL_LOCAL_DATE, // Date without time zone (2024-12-25)
    VAL_LOCAL_TIME, // Time without date or time zone (15:30:45)
    VAL_LOCAL_DATETIME, // Date and time without time zone (2024-12-25T15:30:45)
    VAL_ZONED_DATETIME, // Date and time with time zone (2024-12-25T15:30:45-05:00[America/New_York])
    VAL_INSTANT, // Point in time (Unix timestamp with nanoseconds)
    VAL_DURATION, // Time-based amount (2 hours, 30 minutes)
    VAL_PERIOD // Date-based amount (2 years, 3 months, 5 days)
} value_type;

// Forward declaration for range, iterator, bound method, class, and date/time structures
typedef struct range range_t;
typedef struct iterator iterator_t;
typedef struct bound_method bound_method_t;
typedef struct class class_t;
typedef struct local_date local_date_t;
typedef struct local_time local_time_t;
typedef struct local_datetime local_datetime_t;
typedef struct zoned_datetime zoned_datetime_t;
typedef struct instant instant_t;
typedef struct duration duration_t;
typedef struct period period_t;

// VM value structure
typedef struct value {
    value_type type;
    union {
        int boolean;
        int32_t int32; // 32-bit integer (direct storage)
        di_int bigint; // Arbitrary precision integer (ref-counted)
        double number; // Floating point
        ds_string string; // Using dynamic_string.h!
        ds_builder string_builder; // String builder (using dynamic_string.h!)
        da_array array; // Using dynamic_array.h!
        do_object object; // Using dynamic_object.h!
        class_t* class; // Class definition pointer
        range_t* range; // Range object pointer
        iterator_t* iterator; // Iterator object pointer
        db_buffer buffer; // Buffer data (using dynamic_buffer.h!)
        db_builder builder; // Buffer builder handle (reference counted)
        db_reader reader; // Buffer reader handle (reference counted)
        struct function* function;
        struct closure* closure;
        native_t native; // Native C function pointer
        bound_method_t* bound_method; // Bound method (method + receiver)
        // Date/Time types
        local_date_t* local_date; // Date without time zone
        local_time_t* local_time; // Time without date or time zone
        local_datetime_t* local_datetime; // Date and time without time zone
        zoned_datetime_t* zoned_datetime; // Date and time with time zone
        instant_t* instant; // Point in time (Unix timestamp)
        duration_t* duration; // Time-based amount
        period_t* period; // Date-based amount
    } as;
    value_t* class; // For object instances: pointer to their class value (NULL for non-instances)
    debug_location* debug; // Debug info for error reporting (NULL when disabled)
} value_t;

// Range structure for range expressions (1..10, 1..<10)
struct range {
    int ref_count; // Reference count for memory management
    value_t start; // Starting value
    value_t end; // Ending value
    int exclusive; // 1 for ..< (exclusive), 0 for .. (inclusive)
};

// Iterator types
typedef enum {
    ITER_ARRAY, // Array iterator
    ITER_RANGE // Range iterator
} iterator_type;

// Iterator structure for unified iteration over arrays, ranges, etc.
struct iterator {
    size_t ref_count; // Reference counting for memory management
    iterator_type type;
    union {
        struct {
            da_array array; // Array being iterated
            size_t index; // Current index
        } array_iter;
        struct {
            value_t current; // Current value
            value_t end; // End value
            int exclusive; // Whether end is exclusive
            int finished; // Whether iteration is complete
        } range_iter;
    } data;
};

// Bound method structure (method bound to a specific receiver)
struct bound_method {
    int ref_count; // Reference count for memory management
    value_t receiver; // The object this method is bound to
    native_t method; // The method function pointer
};


// Class structure (prototype holder for prototypal inheritance)
struct class {
    int ref_count; // Reference count for memory management
    char* name; // Class name
    do_object properties; // Prototype properties
    value_t (*factory)(value_t* args, int arg_count); // Factory function for creating instances (NULL if not callable)
};

// Date/Time structures with reference counting

// Local Date structure (date without time zone)
struct local_date {
    int ref_count; // Reference count for memory management
    int32_t year; // Year (e.g., 2024)
    uint8_t month; // Month (1-12)
    uint8_t day; // Day (1-31)
    uint32_t epoch_day; // Days since 1970-01-01 (cached for performance)
};

// Local Time structure (time without date or time zone)
struct local_time {
    int ref_count; // Reference count for memory management
    uint8_t hour; // Hour (0-23)
    uint8_t minute; // Minute (0-59)
    uint8_t second; // Second (0-59)
    uint16_t millis; // Milliseconds (0-999)
    uint32_t nanos; // Nanoseconds (0-999999999, total including millis)
};

// Local DateTime structure (date and time without time zone)
struct local_datetime {
    int ref_count; // Reference count for memory management
    local_date_t* date; // Date component
    local_time_t* time; // Time component
};

// Zoned DateTime structure (date and time with time zone)
struct zoned_datetime {
    int ref_count; // Reference count for memory management
    local_datetime_t* dt; // Local date-time component
    char* zone_id; // Timezone identifier (e.g., "America/New_York")
    int16_t offset_minutes; // UTC offset in minutes (e.g., -300 for EST)
    bool is_dst; // Whether DST is active
};

// Instant structure (point in time as Unix timestamp)
struct instant {
    int ref_count; // Reference count for memory management
    int64_t epoch_seconds; // Seconds since Unix epoch
    uint32_t nanos; // Nanosecond adjustment (0-999999999)
};

// Duration structure (time-based amount)
struct duration {
    int ref_count; // Reference count for memory management
    int64_t seconds; // Total seconds (can be negative)
    int32_t nanos; // Nanosecond adjustment (0-999999999, sign matches seconds)
};

// Period structure (date-based amount)
struct period {
    int ref_count; // Reference count for memory management
    int32_t years; // Years (can be negative)
    int32_t months; // Months (can be negative)
    int32_t days; // Days (can be negative)
};

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

vm_result vm_execute(slate_vm* vm, function_t* function);
vm_result vm_interpret(slate_vm* vm, const char* source);

// Value memory management
value_t vm_retain(value_t value);
void vm_release(value_t value);

// Stack operations
void vm_push(slate_vm* vm, value_t value);
value_t vm_pop(slate_vm* vm);
value_t vm_peek(slate_vm* vm, int distance);

// Value creation functions
value_t make_null(void);
value_t make_undefined(void);
value_t make_boolean(int value);
value_t make_int32(int32_t value);
value_t make_bigint(di_int big);
value_t make_number(double value);
value_t make_string(const char* value);
value_t make_string_ds(ds_string str);
value_t make_string_builder(ds_builder builder);
value_t make_array(da_array array);
value_t make_object(do_object object);
value_t make_class(const char* name, do_object properties);
value_t make_range(value_t start, value_t end, int exclusive);
value_t make_iterator(iterator_t* iterator);
value_t make_buffer(db_buffer buffer);
value_t make_buffer_builder(db_builder builder);
value_t make_buffer_reader(db_reader reader);
value_t make_function(function_t* function);
value_t make_closure(closure_t* closure);
value_t make_native(native_t func);
value_t make_bound_method(value_t receiver, native_t method);
// Date/Time value factory functions
value_t make_local_date(local_date_t* date);
value_t make_local_time(local_time_t* time);
value_t make_local_datetime(local_datetime_t* datetime);
value_t make_zoned_datetime(zoned_datetime_t* zdt);
value_t make_instant(instant_t* instant);
value_t make_duration(duration_t* duration);
value_t make_period(period_t* period);

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

// Value creation functions with debug info
value_t make_null_with_debug(debug_location* debug);
value_t make_undefined_with_debug(debug_location* debug);
value_t make_boolean_with_debug(int value, debug_location* debug);
value_t make_int32_with_debug(int32_t value, debug_location* debug);
value_t make_bigint_with_debug(di_int big, debug_location* debug);
value_t make_number_with_debug(double value, debug_location* debug);
value_t make_string_with_debug(const char* value, debug_location* debug);
value_t make_string_ds_with_debug(ds_string str, debug_location* debug);
value_t make_string_builder_with_debug(ds_builder builder, debug_location* debug);
value_t make_array_with_debug(da_array array, debug_location* debug);
value_t make_object_with_debug(do_object object, debug_location* debug);
value_t make_class_with_debug(const char* name, do_object properties, debug_location* debug);
value_t make_range_with_debug(value_t start, value_t end, int exclusive, debug_location* debug);
value_t make_iterator_with_debug(iterator_t* iterator, debug_location* debug);
value_t make_buffer_with_debug(db_buffer buffer, debug_location* debug);
value_t make_buffer_builder_with_debug(db_builder builder, debug_location* debug);
value_t make_buffer_reader_with_debug(db_reader reader, debug_location* debug);
value_t make_function_with_debug(function_t* function, debug_location* debug);
value_t make_closure_with_debug(closure_t* closure, debug_location* debug);
value_t make_builtin_with_debug(void* builtin_func, debug_location* debug);
value_t make_bound_method_with_debug(value_t receiver, native_t method, debug_location* debug);

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text);
debug_location* debug_location_copy(const debug_location* debug);
void debug_location_free(debug_location* debug);

// Value utility functions
int is_falsy(value_t value);
int is_number(value_t value); // Check if value is numeric (int32, bigint, or number)
int values_equal(value_t a, value_t b);
void print_value(slate_vm* vm, value_t value);
void print_for_builtin(slate_vm* vm, value_t value);
void free_value(value_t value);
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
