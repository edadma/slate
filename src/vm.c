#include "vm.h"
#include "builtins.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h" // For debug_info functions

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
bitty_vm* vm_create(void) {
    bitty_vm* vm = malloc(sizeof(bitty_vm));
    if (!vm)
        return NULL;

    vm->stack = malloc(sizeof(value_t) * STACK_MAX);
    vm->frames = malloc(sizeof(call_frame) * FRAMES_MAX);
    vm->constants = malloc(sizeof(value_t) * CONSTANTS_MAX);

    if (!vm->stack || !vm->frames || !vm->constants) {
        vm_destroy(vm);
        return NULL;
    }

    vm->stack_capacity = STACK_MAX;
    vm->frame_capacity = FRAMES_MAX;
    vm->constant_capacity = CONSTANTS_MAX;

    // Create globals object - bit_value will be stored as property values
    vm->globals = do_create(NULL); // No release function yet
    if (!vm->globals) {
        vm_destroy(vm);
        return NULL;
    }
    
    // Initialize built-in functions
    builtins_init(vm);
    
    // Initialize result register to undefined
    vm->result = make_undefined();
    
    // Initialize debug location
    vm->current_debug = NULL;
    
    // Initialize command line arguments (empty by default)
    vm->argc = 0;
    vm->argv = NULL;

    vm_reset(vm);
    return vm;
}

bitty_vm* vm_create_with_args(int argc, char** argv) {
    bitty_vm* vm = vm_create();
    if (vm) {
        vm->argc = argc;
        vm->argv = argv;
    }
    return vm;
}

void vm_destroy(bitty_vm* vm) {
    if (!vm)
        return;

    // Free constants
    for (size_t i = 0; i < vm->constant_count; i++) {
        free_value(vm->constants[i]);
    }

    free(vm->stack);
    free(vm->frames);
    free(vm->constants);
    do_release(&vm->globals);
    
    // Release result register
    free_value(vm->result);
    
    // Clean up debug location
    debug_location_free(vm->current_debug);
    
    free(vm);
}

void vm_reset(bitty_vm* vm) {
    if (!vm)
        return;

    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}

// Value memory management
value_t vm_retain(value_t value) {
    if (value.type == VAL_STRING) {
        value.as.string = ds_retain(value.as.string);
    } else if (value.type == VAL_ARRAY) {
        value.as.array = da_retain(value.as.array);  
    } else if (value.type == VAL_OBJECT) {
        value.as.object = do_retain(value.as.object);
    } else if (value.type == VAL_BIGINT) {
        value.as.bigint = db_retain(value.as.bigint);
    }
    return value;
}

void vm_release(value_t value) {
    if (value.type == VAL_STRING) {
        ds_release(&value.as.string);
    } else if (value.type == VAL_ARRAY) {
        da_release(&value.as.array);
    } else if (value.type == VAL_OBJECT) {
        do_release(&value.as.object);
    } else if (value.type == VAL_BIGINT) {
        db_release(&value.as.bigint);
    }
}

// Stack operations
void vm_push(bitty_vm* vm, value_t value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(bitty_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(bitty_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }

// Value creation functions
value_t make_null(void) {
    value_t value;
    value.type = VAL_NULL;
    value.debug = NULL;
    return value;
}

value_t make_undefined(void) {
    value_t value;
    value.type = VAL_UNDEFINED;
    value.debug = NULL;
    return value;
}

value_t make_boolean(int val) {
    value_t value;
    value.type = VAL_BOOLEAN;
    value.as.boolean = val;
    value.debug = NULL;
    return value;
}

value_t make_int32(int32_t val) {
    value_t value;
    value.type = VAL_INT32;
    value.as.int32 = val;
    value.debug = NULL;
    return value;
}

value_t make_bigint(db_bigint big) {
    value_t value;
    value.type = VAL_BIGINT;
    value.as.bigint = big;
    value.debug = NULL;
    return value;
}

value_t make_number(double val) {
    value_t value;
    value.type = VAL_NUMBER;
    value.as.number = val;
    value.debug = NULL;
    return value;
}

value_t make_string(const char* val) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = ds_new(val); // Using dynamic_string.h!
    value.debug = NULL;
    return value;
}

value_t make_string_ds(ds_string str) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = str; // Take ownership of the ds_string
    value.debug = NULL;
    return value;
}

value_t make_array(da_array array) {
    value_t value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    value.debug = NULL;
    return value;
}

value_t make_object(do_object object) {
    value_t value;
    value.type = VAL_OBJECT;
    value.as.object = object;
    value.debug = NULL;
    return value;
}

value_t make_function(function_t* function) {
    value_t value;
    value.type = VAL_FUNCTION;
    value.as.function = function;
    value.debug = NULL;
    return value;
}

value_t make_closure(closure_t* closure) {
    value_t value;
    value.type = VAL_CLOSURE;
    value.as.closure = closure;
    value.debug = NULL;
    return value;
}

value_t make_builtin(void* builtin_func) {
    value_t value;
    value.type = VAL_BUILTIN;
    value.as.builtin = builtin_func;
    value.debug = NULL;
    return value;
}

// Value creation functions with debug info
value_t make_null_with_debug(debug_location* debug) {
    value_t value = make_null();
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_undefined_with_debug(debug_location* debug) {
    value_t value = make_undefined();
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_boolean_with_debug(int val, debug_location* debug) {
    value_t value = make_boolean(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_int32_with_debug(int32_t val, debug_location* debug) {
    value_t value = make_int32(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_bigint_with_debug(db_bigint big, debug_location* debug) {
    value_t value = make_bigint(big);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_number_with_debug(double val, debug_location* debug) {
    value_t value = make_number(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_string_with_debug(const char* val, debug_location* debug) {
    value_t value = make_string(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_string_ds_with_debug(ds_string str, debug_location* debug) {
    value_t value = make_string_ds(str);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_array_with_debug(da_array array, debug_location* debug) {
    value_t value = make_array(array);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_object_with_debug(do_object object, debug_location* debug) {
    value_t value = make_object(object);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_function_with_debug(function_t* function, debug_location* debug) {
    value_t value = make_function(function);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_closure_with_debug(closure_t* closure, debug_location* debug) {
    value_t value = make_closure(closure);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_builtin_with_debug(void* builtin_func, debug_location* debug) {
    value_t value = make_builtin(builtin_func);
    value.debug = debug_location_copy(debug);
    return value;
}

// Forward declarations
static ds_string value_to_string_representation(value_t value);
static ds_string display_value_to_string(value_t value);

// Context for object property iteration
struct object_string_context {
    ds_string* result_ptr;
    int count;
};

// Callback function for object property iteration
static void object_property_to_string_callback(const char* key, void* data, size_t size, void* ctx) {
    struct object_string_context* context = (struct object_string_context*)ctx;
    ds_string* result_ptr = context->result_ptr;
    
    // Add comma separator for subsequent properties
    if (context->count > 0) {
        ds_string comma = ds_new(", ");
        ds_string temp = ds_concat(*result_ptr, comma);
        ds_release(result_ptr);
        ds_release(&comma);
        *result_ptr = temp;
    }
    
    // Add key: value
    ds_string key_str = ds_new(key);
    ds_string colon = ds_new(": ");
    ds_string temp1 = ds_concat(*result_ptr, key_str);
    ds_string temp2 = ds_concat(temp1, colon);
    ds_release(result_ptr);
    ds_release(&key_str);
    ds_release(&colon);
    ds_release(&temp1);
    
    // Convert value to string (assuming it's a value_t)
    if (size == sizeof(value_t)) {
        value_t* val = (value_t*)data;
        ds_string val_str = display_value_to_string(*val);
        ds_string temp3 = ds_concat(temp2, val_str);
        ds_release(&temp2);
        ds_release(&val_str);
        *result_ptr = temp3;
    } else {
        ds_string unknown = ds_new("?");
        ds_string temp3 = ds_concat(temp2, unknown);
        ds_release(&temp2);
        ds_release(&unknown);
        *result_ptr = temp3;
    }
    
    context->count++;
}

// Helper function to convert any value to string representation for concatenation
static ds_string value_to_string_representation(value_t value) {
    switch (value.type) {
    case VAL_STRING:
        return ds_retain(value.as.string);
    case VAL_INT32: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", value.as.int32);
        return ds_new(buffer);
    }
    case VAL_BIGINT: {
        char* str = db_to_string(value.as.bigint, 10);
        if (str) {
            ds_string result = ds_new(str);
            free(str);
            return result;
        } else {
            return ds_new("<bigint>");
        }
    }
    case VAL_NUMBER: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6g", value.as.number);
        return ds_new(buffer);
    }
    case VAL_BOOLEAN:
        return ds_new(value.as.boolean ? "true" : "false");
    case VAL_UNDEFINED:
        return ds_new("undefined");
    case VAL_NULL:
        return ds_new("null");
    case VAL_ARRAY: {
        // Create string representation like [1, 2, 3]
        ds_string result = ds_new("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0) {
                    ds_string comma = ds_new(", ");
                    ds_string temp = ds_concat(result, comma);
                    ds_release(&result);
                    ds_release(&comma);
                    result = temp;
                }
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    ds_string element_str = display_value_to_string(*element);
                    ds_string temp = ds_concat(result, element_str);
                    ds_release(&result);
                    ds_release(&element_str);
                    result = temp;
                }
            }
        }
        ds_string bracket = ds_new("]");
        ds_string temp = ds_concat(result, bracket);
        ds_release(&result);
        ds_release(&bracket);
        return temp;
    }
    case VAL_OBJECT: {
        // Create string representation like {key: value, key2: value2}
        ds_string result = ds_new("{");
        if (value.as.object) {
            // Use a context structure to track iteration state
            struct object_string_context context = { &result, 0 };
            
            // Iterate through properties using do_foreach_property
            do_foreach_property(value.as.object, object_property_to_string_callback, &context);
        }
        ds_string bracket = ds_new("}");
        ds_string temp = ds_concat(result, bracket);
        ds_release(&result);
        ds_release(&bracket);
        return temp;
    }
    case VAL_FUNCTION:
        return ds_new("{Function}");
    case VAL_CLOSURE:
        return ds_new("{Closure}");
    default:
        return ds_new("{Unknown}");
    }
}

// Helper function to convert values to display string (with quotes for strings, for use inside aggregates)
static ds_string display_value_to_string(value_t value) {
    switch (value.type) {
    case VAL_STRING: {
        // Add quotes around strings for display inside aggregates
        ds_string quoted = ds_new("\"");
        if (value.as.string) {
            ds_string temp1 = ds_concat(quoted, value.as.string);
            ds_release(&quoted);
            quoted = temp1;
        }
        ds_string end_quote = ds_new("\"");
        ds_string temp2 = ds_concat(quoted, end_quote);
        ds_release(&quoted);
        ds_release(&end_quote);
        return temp2;
    }
    default:
        // For all other types, use the regular representation
        return value_to_string_representation(value);
    }
}

// Print function specifically for the print() builtin - shows strings without quotes
void print_for_builtin(value_t value) {
    switch (value.type) {
    case VAL_STRING:
        // Print strings without quotes for direct printing
        printf("%s", value.as.string ? value.as.string : "");
        break;
    default: {
        // For aggregates and other types, use the string representation and print it
        ds_string str = value_to_string_representation(value);
        printf("%s", str);
        ds_release(&str);
        break;
    }
    }
}

// Value utility functions
int is_falsy(value_t value) {
    switch (value.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return !value.as.boolean;
    case VAL_INT32:
        return value.as.int32 == 0;
    case VAL_BIGINT:
        return db_is_zero(value.as.bigint);
    case VAL_NUMBER:
        return value.as.number == 0.0;
    case VAL_STRING:
        return value.as.string == NULL || strlen(value.as.string) == 0;
    default:
        return 0;
    }
}

int values_equal(value_t a, value_t b) {
    // Handle cross-type numeric comparisons
    if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
        (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
        // Convert to common type for comparison
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            return a.as.int32 == b.as.int32;
        } else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            return db_equal(a.as.bigint, b.as.bigint);
        } else if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
            return a.as.number == b.as.number;
        }
        // Cross-type comparisons - convert to common type
        // For now, convert to double for simplicity
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                       (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                       a.as.number;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                       (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                       b.as.number;
        return a_val == b_val;
    }
    
    if (a.type != b.type)
        return 0;

    switch (a.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return a.as.boolean == b.as.boolean;
    case VAL_STRING:
        if (a.as.string == NULL && b.as.string == NULL)
            return 1;
        if (a.as.string == NULL || b.as.string == NULL)
            return 0;
        return strcmp(a.as.string, b.as.string) == 0; // DS strings work with strcmp!
    case VAL_ARRAY:
        return a.as.array == b.as.array;
    case VAL_OBJECT:
        return a.as.object == b.as.object;
    case VAL_FUNCTION:
        return a.as.function == b.as.function;
    case VAL_CLOSURE:
        return a.as.closure == b.as.closure;
    case VAL_BUILTIN:
        return a.as.builtin == b.as.builtin;
    default:
        return 0;
    }
}

void print_value(value_t value) {
    switch (value.type) {
    case VAL_NULL:
        printf("null");
        break;
    case VAL_UNDEFINED:
        printf("undefined");
        break;
    case VAL_BOOLEAN:
        printf(value.as.boolean ? "true" : "false");
        break;
    case VAL_INT32:
        printf("%d", value.as.int32);
        break;
    case VAL_BIGINT: {
        char* str = db_to_string(value.as.bigint, 10);
        if (str) {
            printf("%s", str);
            free(str);
        } else {
            printf("<bigint>");
        }
        break;
    }
    case VAL_NUMBER:
        printf("%.6g", value.as.number);
        break;
    case VAL_STRING:
        printf("\"%s\"", value.as.string ? value.as.string : ""); // DS strings work directly!
        break;
    case VAL_ARRAY: {
        printf("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0)
                    printf(", ");
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    print_value(*element);
                } else {
                    printf("null");
                }
            }
        }
        printf("]");
        break;
    }
    case VAL_OBJECT: {
        printf("{");
        if (value.as.object) {
            // For now, just show it's an object - full object printing would be more complex
            printf("Object");
        }
        printf("}");
        break;
    }
    case VAL_FUNCTION:
        printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
        break;
    case VAL_CLOSURE:
        printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
        break;
    case VAL_BUILTIN:
        printf("<builtin function>");
        break;
    }
}

void free_value(value_t value) {
    // Clean up debug info first
    debug_location_free(value.debug);
    
    switch (value.type) {
    case VAL_NULL:
    case VAL_UNDEFINED:
    case VAL_BOOLEAN:
    case VAL_NUMBER:
        // No cleanup needed for these types
        break;
    case VAL_STRING: {
        ds_string temp = value.as.string;
        ds_release(&temp); // DS cleanup with reference counting!
        break;
    }
    case VAL_ARRAY: {
        da_array temp = value.as.array;
        da_release(&temp); // DA cleanup with reference counting!
        break;
    }
    case VAL_OBJECT: {
        do_object temp = value.as.object;
        do_release(&temp); // DO cleanup with reference counting!
        break;
    }
    case VAL_FUNCTION:
        function_destroy(value.as.function);
        break;
    case VAL_CLOSURE:
        closure_destroy(value.as.closure);
        break;
    case VAL_BUILTIN:
        // No cleanup needed for builtin function pointers
        break;
    default:
        // No cleanup needed for basic types
        break;
    }
}

// Constant pool management
size_t vm_add_constant(bitty_vm* vm, value_t value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(bitty_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
}

// Note: Object operations now use dynamic_object.h
// No separate functions needed

// Note: Array operations now use dynamic_array.h
// No separate functions needed

// Function operations
function_t* function_create(const char* name) {
    function_t* function = malloc(sizeof(function_t));
    if (!function)
        return NULL;

    function->bytecode = NULL;
    function->bytecode_length = 0;
    function->constants = NULL;
    function->constant_count = 0;
    function->parameter_names = NULL;
    function->parameter_count = 0;
    function->local_count = 0;
    function->name = name ? strdup(name) : NULL;
    function->debug = NULL; // Initialize debug info

    return function;
}

void function_destroy(function_t* function) {
    if (!function)
        return;

    free(function->bytecode);

    for (size_t i = 0; i < function->constant_count; i++) {
        free_value(function->constants[i]);
    }
    free(function->constants);

    for (size_t i = 0; i < function->parameter_count; i++) {
        free(function->parameter_names[i]);
    }
    free(function->parameter_names);

    debug_info_destroy(function->debug);

    free(function->name);
    free(function);
}

closure_t* closure_create(function_t* function) {
    closure_t* closure = malloc(sizeof(closure_t));
    if (!closure)
        return NULL;

    closure->function = function;
    closure->upvalues = NULL;
    closure->upvalue_count = 0;

    return closure;
}

void closure_destroy(closure_t* closure) {
    if (!closure)
        return;

    function_destroy(closure->function);
    free(closure->upvalues);
    free(closure);
}

// Bytecode utilities
const char* opcode_name(opcode op) {
    switch (op) {
    case OP_PUSH_CONSTANT:
        return "PUSH_CONSTANT";
    case OP_PUSH_NULL:
        return "PUSH_NULL";
    case OP_PUSH_UNDEFINED:
        return "PUSH_UNDEFINED";
    case OP_PUSH_TRUE:
        return "PUSH_TRUE";
    case OP_PUSH_FALSE:
        return "PUSH_FALSE";
    case OP_POP:
        return "POP";
    case OP_DUP:
        return "DUP";
    case OP_SET_RESULT:
        return "SET_RESULT";
    case OP_ADD:
        return "ADD";
    case OP_SUBTRACT:
        return "SUBTRACT";
    case OP_MULTIPLY:
        return "MULTIPLY";
    case OP_DIVIDE:
        return "DIVIDE";
    case OP_MOD:
        return "MOD";
    case OP_POWER:
        return "POWER";
    case OP_NEGATE:
        return "NEGATE";
    case OP_EQUAL:
        return "EQUAL";
    case OP_NOT_EQUAL:
        return "NOT_EQUAL";
    case OP_LESS:
        return "LESS";
    case OP_LESS_EQUAL:
        return "LESS_EQUAL";
    case OP_GREATER:
        return "GREATER";
    case OP_GREATER_EQUAL:
        return "GREATER_EQUAL";
    case OP_NOT:
        return "NOT";
    case OP_AND:
        return "AND";
    case OP_OR:
        return "OR";
    case OP_BITWISE_AND:
        return "BITWISE_AND";
    case OP_BITWISE_OR:
        return "BITWISE_OR";
    case OP_BITWISE_XOR:
        return "BITWISE_XOR";
    case OP_BITWISE_NOT:
        return "BITWISE_NOT";
    case OP_LEFT_SHIFT:
        return "LEFT_SHIFT";
    case OP_RIGHT_SHIFT:
        return "RIGHT_SHIFT";
    case OP_LOGICAL_RIGHT_SHIFT:
        return "LOGICAL_RIGHT_SHIFT";
    case OP_FLOOR_DIV:
        return "FLOOR_DIV";
    case OP_INCREMENT:
        return "INCREMENT";
    case OP_DECREMENT:
        return "DECREMENT";
    case OP_GET_LOCAL:
        return "GET_LOCAL";
    case OP_SET_LOCAL:
        return "SET_LOCAL";
    case OP_GET_GLOBAL:
        return "GET_GLOBAL";
    case OP_SET_GLOBAL:
        return "SET_GLOBAL";
    case OP_DEFINE_GLOBAL:
        return "DEFINE_GLOBAL";
    case OP_GET_PROPERTY:
        return "GET_PROPERTY";
    case OP_SET_PROPERTY:
        return "SET_PROPERTY";
    case OP_GET_INDEX:
        return "GET_INDEX";
    case OP_SET_INDEX:
        return "SET_INDEX";
    case OP_BUILD_ARRAY:
        return "BUILD_ARRAY";
    case OP_BUILD_OBJECT:
        return "BUILD_OBJECT";
    case OP_CLOSURE:
        return "CLOSURE";
    case OP_CALL:
        return "CALL";
    case OP_RETURN:
        return "RETURN";
    case OP_JUMP:
        return "JUMP";
    case OP_JUMP_IF_FALSE:
        return "JUMP_IF_FALSE";
    case OP_JUMP_IF_TRUE:
        return "JUMP_IF_TRUE";
    case OP_LOOP:
        return "LOOP";
    case OP_SET_DEBUG_LOCATION:
        return "SET_DEBUG_LOCATION";
    case OP_CLEAR_DEBUG_LOCATION:
        return "CLEAR_DEBUG_LOCATION";
    case OP_HALT:
        return "HALT";
    default:
        return "UNKNOWN";
    }
}

// Basic VM execution (stub for now)
vm_result vm_execute(bitty_vm* vm, function_t* function) {
    if (!vm || !function)
        return VM_RUNTIME_ERROR;

    // Clear the stack at the start of each execution (important for REPL)
    vm->stack_top = vm->stack;

    // Set up initial call frame
    if (vm->frame_count >= vm->frame_capacity) {
        return VM_STACK_OVERFLOW;
    }

    call_frame* frame = &vm->frames[vm->frame_count++];
    closure_t* closure = closure_create(function); // Simple closure wrapper
    frame->closure = closure;
    frame->ip = function->bytecode;
    frame->slots = vm->stack_top;

    vm->bytecode = function->bytecode;
    vm->ip = function->bytecode;

    // Basic execution loop (simplified)
    for (;;) {
        vm->current_instruction = vm->ip;  // Store instruction start for error reporting
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2; // Skip the operand bytes
            
            // Create value with current debug info
            value_t val = function->constants[constant];
            if (vm->current_debug) {
                val.debug = debug_location_copy(vm->current_debug);
            }
            vm_push(vm, val);
            break;
        }

        case OP_PUSH_NULL:
            vm_push(vm, make_null_with_debug(vm->current_debug));
            break;

        case OP_PUSH_UNDEFINED:
            vm_push(vm, make_undefined_with_debug(vm->current_debug));
            break;

        case OP_PUSH_TRUE:
            vm_push(vm, make_boolean_with_debug(1, vm->current_debug));
            break;

        case OP_PUSH_FALSE:
            vm_push(vm, make_boolean_with_debug(0, vm->current_debug));
            break;

        case OP_POP: {
            vm_pop(vm);
            // Clean pop - no special behavior
            break;
        }

        case OP_DUP: {
            value_t value = vm_peek(vm, 0);
            vm_push(vm, value); // vm_push will handle the retain
            break;
        }
        
        case OP_SET_RESULT: {
            // Pop value from stack and store in result register
            // Release old result value first
            vm_release(vm->result);
            vm->result = vm_pop(vm);
            // No need to retain - vm_pop transfers ownership
            break;
        }

        case OP_ADD: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);

            // String concatenation (if either operand is a string)
            if (a.type == VAL_STRING || b.type == VAL_STRING) {
                // Convert both to strings using helper function
                ds_string str_a = value_to_string_representation(a);
                ds_string str_b = value_to_string_representation(b);

                // Concatenate using DS library
                ds_string result = ds_append(str_a, str_b);
                vm_push(vm, make_string_ds_with_debug(result, a.debug));

                // Clean up temporary strings
                ds_release(&str_a);
                ds_release(&str_b);
            }
            // Array concatenation (if both operands are arrays)
            else if (a.type == VAL_ARRAY && b.type == VAL_ARRAY) {
                // Create new array for concatenation result
                da_array result_array = da_new(sizeof(value_t));
                
                // Add all elements from left array
                size_t a_len = da_length(a.as.array);
                for (size_t i = 0; i < a_len; i++) {
                    value_t* elem = (value_t*)da_get(a.as.array, i);
                    value_t retained_elem = vm_retain(*elem);
                    da_push(result_array, &retained_elem);
                }
                
                // Add all elements from right array  
                size_t b_len = da_length(b.as.array);
                for (size_t i = 0; i < b_len; i++) {
                    value_t* elem = (value_t*)da_get(b.as.array, i);
                    value_t retained_elem = vm_retain(*elem);
                    da_push(result_array, &retained_elem);
                }
                
                vm_push(vm, make_array_with_debug(result_array, a.debug));
            }
            // Numeric addition - handle all numeric type combinations
            else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                     (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // int32 + int32 with overflow detection
                if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                    int32_t result;
                    if (db_add_overflow_int32(a.as.int32, b.as.int32, &result)) {
                        vm_push(vm, make_int32_with_debug(result, a.debug));
                    } else {
                        // Overflow - promote to BigInt
                        int64_t big_result = (int64_t)a.as.int32 + (int64_t)b.as.int32;
                        db_bigint big = db_from_int64(big_result);
                        vm_push(vm, make_bigint_with_debug(big, a.debug));
                    }
                }
                // BigInt + BigInt
                else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
                    db_bigint result = db_add(a.as.bigint, b.as.bigint);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // int32 + BigInt
                else if (a.type == VAL_INT32 && b.type == VAL_BIGINT) {
                    db_bigint result = db_add_int32(b.as.bigint, a.as.int32);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // BigInt + int32
                else if (a.type == VAL_BIGINT && b.type == VAL_INT32) {
                    db_bigint result = db_add_int32(a.as.bigint, b.as.int32);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // Mixed with floating point - convert to double
                else {
                    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                                   a.as.number;
                    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                                   b.as.number;
                    vm_push(vm, make_number_with_debug(a_val + b_val, a.debug));
                }
            }
            else {
                // Find the first non-numeric operand for error location
                debug_location* error_debug = NULL;
                
                if (a.type != VAL_NUMBER) {
                    // Left operand is the first non-numeric
                    error_debug = a.debug;
                } else {
                    // Right operand must be non-numeric  
                    error_debug = b.debug;
                }
                
                vm_runtime_error_with_values(vm, "Cannot add %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }


        case OP_SUBTRACT: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // int32 - int32 with overflow detection
                if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                    int32_t result;
                    if (db_subtract_overflow_int32(a.as.int32, b.as.int32, &result)) {
                        vm_push(vm, make_int32_with_debug(result, a.debug));
                    } else {
                        // Overflow - promote to BigInt
                        int64_t big_result = (int64_t)a.as.int32 - (int64_t)b.as.int32;
                        db_bigint big = db_from_int64(big_result);
                        vm_push(vm, make_bigint_with_debug(big, a.debug));
                    }
                }
                // Mixed with floating point - convert to double
                else {
                    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                                   a.as.number;
                    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                                   b.as.number;
                    vm_push(vm, make_number_with_debug(a_val - b_val, a.debug));
                }
            } else {
                // For subtraction, determine which operand is problematic
                debug_location* error_debug = NULL;
                if ((a.type != VAL_INT32 && a.type != VAL_BIGINT && a.type != VAL_NUMBER) && 
                    (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                    error_debug = a.debug;  // Left operand is problematic
                } else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) && 
                          (b.type != VAL_INT32 && b.type != VAL_BIGINT && b.type != VAL_NUMBER)) {
                    error_debug = b.debug;  // Right operand is problematic
                } else {
                    error_debug = a.debug;  // Both problematic, use left
                }
                
                vm_runtime_error_with_values(vm, "Cannot subtract %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_MULTIPLY: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // int32 * int32 with overflow detection
                if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                    int32_t result;
                    if (db_multiply_overflow_int32(a.as.int32, b.as.int32, &result)) {
                        vm_push(vm, make_int32_with_debug(result, a.debug));
                    } else {
                        // Overflow - promote to BigInt
                        int64_t big_result = (int64_t)a.as.int32 * (int64_t)b.as.int32;
                        db_bigint big = db_from_int64(big_result);
                        vm_push(vm, make_bigint_with_debug(big, a.debug));
                    }
                }
                // BigInt * BigInt
                else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
                    db_bigint result = db_multiply(a.as.bigint, b.as.bigint);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // int32 * BigInt
                else if (a.type == VAL_INT32 && b.type == VAL_BIGINT) {
                    db_bigint result = db_multiply_int32(b.as.bigint, a.as.int32);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // BigInt * int32
                else if (a.type == VAL_BIGINT && b.type == VAL_INT32) {
                    db_bigint result = db_multiply_int32(a.as.bigint, b.as.int32);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // Mixed with floating point - convert to double
                else {
                    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                                   a.as.number;
                    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                                   b.as.number;
                    vm_push(vm, make_number_with_debug(a_val * b_val, a.debug));
                }
            }
            else {
                // Find the first non-numeric operand for error location
                debug_location* error_debug = NULL;
                if ((a.type != VAL_INT32 && a.type != VAL_BIGINT && a.type != VAL_NUMBER) && 
                    (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                    error_debug = a.debug;  // Left operand is problematic
                } else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) && 
                          (b.type != VAL_INT32 && b.type != VAL_BIGINT && b.type != VAL_NUMBER)) {
                    error_debug = b.debug;  // Right operand is problematic
                } else {
                    error_debug = a.debug;  // Both problematic, use left
                }
                
                vm_runtime_error_with_values(vm, "Cannot multiply %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_DIVIDE: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Check for division by zero
                bool is_zero = false;
                if (b.type == VAL_INT32 && b.as.int32 == 0) is_zero = true;
                else if (b.type == VAL_BIGINT && db_is_zero(b.as.bigint)) is_zero = true;
                else if (b.type == VAL_NUMBER && b.as.number == 0) is_zero = true;
                
                if (is_zero) {
                    vm_runtime_error_with_values(vm, "Division by zero", &a, &b, b.debug);
                    vm_release(a);
                    vm_release(b);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                
                // Division always produces floating point result for simplicity
                // (matches Python 3 behavior: 5 / 2 = 2.5)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                vm_push(vm, make_number_with_debug(a_val / b_val, a.debug));
            }
            else {
                // Find the first non-numeric operand for error location
                debug_location* error_debug = NULL;
                if ((a.type != VAL_INT32 && a.type != VAL_BIGINT && a.type != VAL_NUMBER) && 
                    (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                    error_debug = a.debug;  // Left operand is problematic
                } else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) && 
                          (b.type != VAL_INT32 && b.type != VAL_BIGINT && b.type != VAL_NUMBER)) {
                    error_debug = b.debug;  // Right operand is problematic
                } else {
                    error_debug = a.debug;  // Both problematic, use left
                }
                
                vm_runtime_error_with_values(vm, "Cannot divide %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_MOD: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Check for modulo by zero
                bool is_zero = false;
                if (b.type == VAL_INT32 && b.as.int32 == 0) is_zero = true;
                else if (b.type == VAL_BIGINT && db_is_zero(b.as.bigint)) is_zero = true;
                else if (b.type == VAL_NUMBER && b.as.number == 0) is_zero = true;
                
                if (is_zero) {
                    vm_runtime_error_with_values(vm, "Modulo by zero", &a, &b, b.debug);
                    vm_release(a);
                    vm_release(b);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                
                // int32 % int32
                if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                    // No overflow possible with modulo
                    vm_push(vm, make_int32_with_debug(a.as.int32 % b.as.int32, a.debug));
                }
                // BigInt % BigInt
                else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
                    db_bigint result = db_mod(a.as.bigint, b.as.bigint);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // int32 % BigInt
                else if (a.type == VAL_INT32 && b.type == VAL_BIGINT) {
                    db_bigint a_big = db_from_int32(a.as.int32);
                    db_bigint result = db_mod(a_big, b.as.bigint);
                    db_release(&a_big);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // BigInt % int32
                else if (a.type == VAL_BIGINT && b.type == VAL_INT32) {
                    db_bigint b_big = db_from_int32(b.as.int32);
                    db_bigint result = db_mod(a.as.bigint, b_big);
                    db_release(&b_big);
                    vm_push(vm, make_bigint_with_debug(result, a.debug));
                }
                // Mixed with floating point - use fmod
                else {
                    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                                   a.as.number;
                    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                                   b.as.number;
                    vm_push(vm, make_number_with_debug(fmod(a_val, b_val), a.debug));
                }
            }
            else {
                // Find the first non-numeric operand for error location
                debug_location* error_debug = NULL;
                if ((a.type != VAL_INT32 && a.type != VAL_BIGINT && a.type != VAL_NUMBER) && 
                    (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                    error_debug = a.debug;  // Left operand is problematic
                } else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) && 
                          (b.type != VAL_INT32 && b.type != VAL_BIGINT && b.type != VAL_NUMBER)) {
                    error_debug = b.debug;  // Right operand is problematic
                } else {
                    error_debug = a.debug;  // Both problematic, use left
                }
                
                vm_runtime_error_with_values(vm, "Cannot compute modulo of %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_POWER: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations for power operations
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Convert to double for power calculation (always returns float)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                               
                vm_push(vm, make_number_with_debug(pow(a_val, b_val), a.debug));
            } else {
                // Find the first non-numeric operand for error location
                debug_location* error_debug = NULL;
                if ((a.type != VAL_INT32 && a.type != VAL_BIGINT && a.type != VAL_NUMBER) && 
                    (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                    error_debug = a.debug;  // Left operand is problematic
                } else if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) && 
                          (b.type != VAL_INT32 && b.type != VAL_BIGINT && b.type != VAL_NUMBER)) {
                    error_debug = b.debug;  // Right operand is problematic
                } else {
                    error_debug = a.debug;  // Both problematic, use left
                }
                
                vm_runtime_error_with_values(vm, "Cannot compute power of %s and %s", &a, &b, error_debug);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_NEGATE: {
            value_t a = vm_pop(vm);
            if (a.type == VAL_INT32) {
                // Check for int32 overflow on negation
                if (a.as.int32 == INT32_MIN) {
                    // INT32_MIN negation overflows - promote to BigInt
                    db_bigint big = db_from_int64(-((int64_t)INT32_MIN));
                    vm_push(vm, make_bigint_with_debug(big, a.debug));
                } else {
                    vm_push(vm, make_int32_with_debug(-a.as.int32, a.debug));
                }
            } else if (a.type == VAL_BIGINT) {
                db_bigint negated = db_negate(a.as.bigint);
                vm_push(vm, make_bigint_with_debug(negated, a.debug));
            } else if (a.type == VAL_NUMBER) {
                vm_push(vm, make_number_with_debug(-a.as.number, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Cannot negate %s", &a, NULL, NULL);
                vm_release(a);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            break;
        }

        case OP_NOT: {
            value_t a = vm_pop(vm);
            int result = is_falsy(a);
            vm_push(vm, make_boolean_with_debug(result, a.debug));
            vm_release(a);
            break;
        }

        case OP_EQUAL: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            int result = values_equal(a, b);
            vm_push(vm, make_boolean(result));
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_NOT_EQUAL: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            int result = !values_equal(a, b);
            vm_push(vm, make_boolean(result));
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_LESS: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations for comparison
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Convert both to double for comparison (simple but works)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                               
                vm_push(vm, make_boolean(a_val < b_val));
            } else {
                vm_runtime_error_with_values(vm, "Can only compare numbers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_LESS_EQUAL: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations for comparison
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Convert both to double for comparison (simple but works)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                               
                vm_push(vm, make_boolean(a_val <= b_val));
            } else {
                vm_runtime_error_with_values(vm, "Can only compare numbers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_GREATER: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations for comparison
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Convert both to double for comparison (simple but works)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                               
                vm_push(vm, make_boolean(a_val > b_val));
            } else {
                vm_runtime_error_with_values(vm, "Can only compare numbers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_GREATER_EQUAL: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Handle all numeric type combinations for comparison
            if ((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
                (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER)) {
                
                // Convert both to double for comparison (simple but works)
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                               (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                               a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                               (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                               b.as.number;
                               
                vm_push(vm, make_boolean(a_val >= b_val));
            } else {
                vm_runtime_error_with_values(vm, "Can only compare numbers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_AND: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Logical AND: if a is falsy, return a, otherwise return b
            if (is_falsy(a)) {
                vm_push(vm, a);
                vm_release(b);
            } else {
                vm_push(vm, b);
                vm_release(a);
            }
            break;
        }

        case OP_OR: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            // Logical OR: if a is truthy, return a, otherwise return b
            if (is_falsy(a)) {
                vm_push(vm, b);
                vm_release(a);
            } else {
                vm_push(vm, a);
                vm_release(b);
            }
            break;
        }

        case OP_BITWISE_AND: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                vm_push(vm, make_int32_with_debug(a.as.int32 & b.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Bitwise AND requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_BITWISE_OR: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                vm_push(vm, make_int32_with_debug(a.as.int32 | b.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Bitwise OR requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_BITWISE_XOR: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                vm_push(vm, make_int32_with_debug(a.as.int32 ^ b.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Bitwise XOR requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_BITWISE_NOT: {
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32) {
                vm_push(vm, make_int32_with_debug(~a.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Bitwise NOT requires integer", &a, NULL, NULL);
                vm_release(a);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            break;
        }

        case OP_LEFT_SHIFT: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                vm_push(vm, make_int32_with_debug(a.as.int32 << b.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Left shift requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_RIGHT_SHIFT: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                // Arithmetic right shift (sign-extending)
                vm_push(vm, make_int32_with_debug(a.as.int32 >> b.as.int32, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Arithmetic right shift requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_LOGICAL_RIGHT_SHIFT: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32 && b.type == VAL_INT32) {
                // Logical right shift (zero-filling)
                // Cast to unsigned to ensure zero-fill behavior
                uint32_t unsigned_a = (uint32_t)a.as.int32;
                uint32_t result = unsigned_a >> b.as.int32;
                vm_push(vm, make_int32_with_debug((int32_t)result, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Logical right shift requires integers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_FLOOR_DIV: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            
            if ((a.type == VAL_INT32 || a.type == VAL_NUMBER) && 
                (b.type == VAL_INT32 || b.type == VAL_NUMBER)) {
                
                // Convert to doubles for division
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 : a.as.number;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 : b.as.number;
                
                if (b_val == 0) {
                    vm_runtime_error_with_values(vm, "Division by zero", &a, &b, NULL);
                    vm_release(a);
                    vm_release(b);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                
                // Floor division - truncate towards negative infinity
                double result = floor(a_val / b_val);
                
                // If result fits in int32, return as int32
                if (result >= INT32_MIN && result <= INT32_MAX) {
                    vm_push(vm, make_int32_with_debug((int32_t)result, a.debug));
                } else {
                    vm_push(vm, make_number_with_debug(result, a.debug));
                }
            } else {
                vm_runtime_error_with_values(vm, "Floor division requires numbers", &a, &b, NULL);
                vm_release(a);
                vm_release(b);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            vm_release(b);
            break;
        }

        case OP_INCREMENT: {
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32) {
                // Check for overflow
                if (a.as.int32 == INT32_MAX) {
                    // Overflow - promote to BigInt
                    db_bigint big = db_from_int64((int64_t)a.as.int32 + 1);
                    vm_push(vm, make_bigint_with_debug(big, a.debug));
                } else {
                    vm_push(vm, make_int32_with_debug(a.as.int32 + 1, a.debug));
                }
            } else if (a.type == VAL_BIGINT) {
                db_bigint one = db_from_int64(1);
                db_bigint result = db_add(a.as.bigint, one);
                db_release(&one);
                vm_push(vm, make_bigint_with_debug(result, a.debug));
            } else if (a.type == VAL_NUMBER) {
                vm_push(vm, make_number_with_debug(a.as.number + 1, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Cannot increment %s", &a, NULL, NULL);
                vm_release(a);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            break;
        }

        case OP_DECREMENT: {
            value_t a = vm_pop(vm);
            
            if (a.type == VAL_INT32) {
                // Check for underflow
                if (a.as.int32 == INT32_MIN) {
                    // Underflow - promote to BigInt
                    db_bigint big = db_from_int64((int64_t)a.as.int32 - 1);
                    vm_push(vm, make_bigint_with_debug(big, a.debug));
                } else {
                    vm_push(vm, make_int32_with_debug(a.as.int32 - 1, a.debug));
                }
            } else if (a.type == VAL_BIGINT) {
                db_bigint one = db_from_int64(1);
                db_bigint result = db_subtract(a.as.bigint, one);
                db_release(&one);
                vm_push(vm, make_bigint_with_debug(result, a.debug));
            } else if (a.type == VAL_NUMBER) {
                vm_push(vm, make_number_with_debug(a.as.number - 1, a.debug));
            } else {
                vm_runtime_error_with_values(vm, "Cannot decrement %s", &a, NULL, NULL);
                vm_release(a);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            vm_release(a);
            break;
        }

        case OP_BUILD_ARRAY: {
            uint16_t count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;

            // Create new dynamic array for bit_value elements
            da_array array = da_new(sizeof(value_t));

            // Collect all elements from stack (they're in reverse order)
            value_t* elements = malloc(sizeof(value_t) * count);
            for (int i = count - 1; i >= 0; i--) {
                elements[i] = vm_pop(vm);
                // Check if trying to store undefined (not a first-class value)
                if (elements[i].type == VAL_UNDEFINED) {
                    vm_runtime_error_with_debug(vm, "Cannot store 'undefined' in array - it is not a value");
                    free(elements);
                    da_release(&array);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
            }

            // Add elements to array in correct order
            for (size_t i = 0; i < count; i++) {
                da_push(array, &elements[i]);
            }
            free(elements);

            vm_push(vm, make_array(array));
            break;
        }

        case OP_BUILD_OBJECT: {
            uint16_t pair_count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;

            // Create new dynamic object
            do_object object = do_create(NULL);  // NULL release function for now
            if (!object) {
                vm_runtime_error_with_debug(vm, "Failed to create object");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }

            // Pop key-value pairs from stack (they're in reverse order)
            for (int i = 0; i < pair_count; i++) {
                value_t value = vm_pop(vm);
                value_t key = vm_pop(vm);
                
                // Check if trying to store undefined (not a first-class value)
                if (value.type == VAL_UNDEFINED) {
                    vm_runtime_error_with_debug(vm, "Cannot store 'undefined' in object - it is not a value");
                    do_release(&object);
                    vm_release(key);
                    vm_release(value);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }

                // Key must be a string
                if (key.type != VAL_STRING) {
                    vm_runtime_error_with_debug(vm, "Object key must be a string");
                    do_release(&object);
                    vm_release(key);
                    vm_release(value);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }

                // Set property in object
                if (do_set(object, key.as.string, &value, sizeof(value_t)) != 0) {
                    vm_runtime_error_with_debug(vm, "Failed to set object property");
                    do_release(&object);
                    vm_release(key);
                    vm_release(value);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }

                // Clean up key (value is now owned by the object)
                vm_release(key);
            }

            vm_push(vm, make_object(object));
            break;
        }

        case OP_GET_INDEX: {
            value_t index = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (object.type == VAL_ARRAY && index.type == VAL_NUMBER) {
                int idx = (int)index.as.number;
                // Check bounds - out of bounds is an error
                if (idx < 0 || idx >= da_length(object.as.array)) {
                    printf("Runtime error: Array index %d out of bounds (length: %d)\n", idx,
                           da_length(object.as.array));
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t* element = (value_t*)da_get(object.as.array, idx);
                vm_push(vm, *element);
            } else if (object.type == VAL_STRING && index.type == VAL_NUMBER) {
                // String indexing
                int idx = (int)index.as.number;
                ds_string str = object.as.string;
                if (idx < 0 || idx >= (int)ds_length(str)) {
                    printf("Runtime error: String index %d out of bounds (length: %d)\n", idx, (int)ds_length(str));
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                char ch = str[idx]; // ds_string is just char*, can index directly
                char result[2] = {ch, '\0'};
                vm_push(vm, make_string(result));
            } else {
                printf("Runtime error: Cannot index non-array/string value\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_SET_INDEX: {
            value_t value = vm_pop(vm);
            value_t index = vm_pop(vm);
            value_t object = vm_pop(vm);

            // Check if trying to store undefined (not a first-class value)
            if (value.type == VAL_UNDEFINED) {
                vm_runtime_error_with_debug(vm, "Cannot store 'undefined' - it is not a value");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }

            if (object.type == VAL_ARRAY && index.type == VAL_NUMBER) {
                size_t idx = (size_t)index.as.number;
                // Set element in dynamic array
                da_set(object.as.array, (int)idx, &value);
                vm_push(vm, value); // Assignment returns the assigned value
            } else {
                printf("Runtime error: Cannot set index on non-array value\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_CALL: {
            uint16_t arg_count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;

            // Pop arguments into temporary array (they're on stack in reverse order)
            value_t* args = NULL;
            if (arg_count > 0) {
                args = malloc(sizeof(value_t) * arg_count);
                for (int i = arg_count - 1; i >= 0; i--) {
                    args[i] = vm_pop(vm);
                    // Check if trying to pass undefined (not a first-class value)
                    if (args[i].type == VAL_UNDEFINED) {
                        vm_runtime_error_with_debug(vm, "Cannot pass 'undefined' as argument - it is not a value");
                        if (args)
                            free(args);
                        vm->frame_count--;
                        closure_destroy(closure);
                        return VM_RUNTIME_ERROR;
                    }
                }
            }

            // Get the callable value (now on top of stack)
            value_t callable = vm_pop(vm);

            // Arrays: act as functions from index to value
            if (callable.type == VAL_ARRAY) {
                if (arg_count != 1) {
                    printf("Runtime error: Array indexing requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t index = args[0];
                if (index.type != VAL_INT32) {
                    printf("Runtime error: Array index must be an integer\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                int idx = index.as.int32;
                // Check bounds - out of bounds is an error
                if (idx < 0 || idx >= da_length(callable.as.array)) {
                    printf("Runtime error: Array index %d out of bounds (length: %d)\n", idx,
                           da_length(callable.as.array));
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t* element = (value_t*)da_get(callable.as.array, idx);
                vm_push(vm, *element);
                if (args)
                    free(args);
            }
            // Strings: act as functions from index to character
            else if (callable.type == VAL_STRING) {
                if (arg_count != 1) {
                    printf("Runtime error: String indexing requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t index = args[0];
                if (index.type != VAL_INT32) {
                    printf("Runtime error: String index must be an integer\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                int idx = index.as.int32;
                ds_string str = callable.as.string;
                if (idx < 0 || idx >= (int)ds_length(str)) {
                    printf("Runtime error: String index %d out of bounds (length: %d)\n", idx, (int)ds_length(str));
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                char ch = str[idx];
                char result[2] = {ch, '\0'};
                vm_push(vm, make_string(result));
                if (args)
                    free(args);
            }
            // Objects: act as functions from property name to value
            else if (callable.type == VAL_OBJECT) {
                if (arg_count != 1) {
                    printf("Runtime error: Object property access requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t key = args[0];
                if (key.type != VAL_STRING) {
                    printf("Runtime error: Object key must be a string\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                // Get property from dynamic object (returns bit_value*)
                value_t* prop_value = (value_t*)do_get(callable.as.object, key.as.string);
                if (prop_value) {
                    vm_push(vm, *prop_value);
                } else {
                    // Missing property returns undefined
                    vm_push(vm, make_undefined());
                }
                if (args)
                    free(args);
            }
            // Built-in functions
            else if (callable.type == VAL_BUILTIN) {
                builtin_func_t builtin_func = (builtin_func_t)callable.as.builtin;
                value_t result = builtin_func(vm, arg_count, args);
                vm_push(vm, result);
                if (args)
                    free(args);
            }
            // Functions: traditional function call (not implemented yet)
            else if (callable.type == VAL_FUNCTION || callable.type == VAL_CLOSURE) {
                printf("Runtime error: Function calls not yet implemented\n");
                if (args)
                    free(args);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            } else {
                // Provide specific error for undefined (likely unknown function)
                if (callable.type == VAL_UNDEFINED) {
                    printf("Runtime error: Unknown function (undefined variable)\n");
                } else {
                    printf("Runtime error: Value is not callable\n");
                }
                if (args)
                    free(args);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_DEFINE_GLOBAL: {
            // Pop the value to store and the variable name constant
            value_t value = vm_pop(vm);
            
            // Note: We allow undefined for variable declarations (var x;)
            // The restriction on undefined only applies to explicit assignments
            
            uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            value_t name_val = function->constants[name_constant];
            if (name_val.type != VAL_STRING) {
                printf("Runtime error: Global variable name must be a string\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            
            // Store in globals object - we need to store a copy of the value
            value_t* stored_value = malloc(sizeof(value_t));
            if (!stored_value) {
                printf("Runtime error: Memory allocation failed\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            *stored_value = value;
            
            // ds_string can be used directly as char* - no ds_cstr needed
            // do_set needs key, data pointer, and size
            do_set(vm->globals, name_val.as.string, stored_value, sizeof(value_t));
            break;
        }
        
        case OP_GET_GLOBAL: {
            uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            value_t name_val = function->constants[name_constant];
            if (name_val.type != VAL_STRING) {
                printf("Runtime error: Global variable name must be a string\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            
            value_t* stored_value = (value_t*)do_get(vm->globals, name_val.as.string);
            if (stored_value) {
                vm_push(vm, *stored_value);
            } else {
                // Create formatted message with dynamic content
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Undefined variable '%s' (if this is a function call, the function doesn't exist)", name_val.as.string);
                vm_runtime_error_with_debug(vm, error_msg);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }
        
        case OP_SET_GLOBAL: {
            // Pop the value to store and get the variable name constant
            value_t value = vm_pop(vm);
            
            // Check if trying to assign undefined (not a first-class value)
            if (value.type == VAL_UNDEFINED) {
                vm_runtime_error_with_debug(vm, "Cannot assign 'undefined' - it is not a value");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            
            uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            value_t name_val = function->constants[name_constant];
            if (name_val.type != VAL_STRING) {
                printf("Runtime error: Global variable name must be a string\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            
            // Check if variable exists
            value_t* stored_value = (value_t*)do_get(vm->globals, name_val.as.string);
            if (stored_value) {
                // Release the old value first (proper reference counting)
                vm_release(*stored_value);
                
                // Update with new value (already popped from stack, so we own it)
                *stored_value = value;
            } else {
                printf("Runtime error: Undefined variable '%s'\n", name_val.as.string);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_GET_PROPERTY: {
            value_t property = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (property.type != VAL_STRING) {
                printf("Runtime error: Property name must be a string\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }

            const char* prop_name = property.as.string;

            if (object.type == VAL_ARRAY) {
                if (strcmp(prop_name, "length") == 0) {
                    vm_push(vm, make_int32((int32_t)da_length(object.as.array)));
                } else {
                    // Invalid property returns undefined, not error
                    vm_push(vm, make_undefined());
                }
            } else if (object.type == VAL_STRING) {
                if (strcmp(prop_name, "length") == 0) {
                    vm_push(vm, make_int32((int32_t)ds_length(object.as.string)));
                } else {
                    // Invalid property returns undefined, not error
                    vm_push(vm, make_undefined());
                }
            } else if (object.type == VAL_OBJECT) {
                // Get property from dynamic object (returns bit_value*)
                value_t* prop_value = (value_t*)do_get(object.as.object, prop_name);
                if (prop_value) {
                    vm_push(vm, *prop_value);
                } else {
                    // Missing property returns undefined
                    vm_push(vm, make_undefined());
                }
            } else {
                // Property access on invalid types returns undefined
                vm_push(vm, make_undefined());
            }
            break;
        }
        
        case OP_JUMP: {
            uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            // Handle both forward and backward jumps
            if (offset > 32767) { // If > 2^15-1, treat as negative (backward jump)
                int16_t backward_offset = (int16_t)offset;
                vm->ip += backward_offset;
            } else {
                vm->ip += offset; // Forward jump
            }
            break;
        }
        
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            value_t condition = vm_peek(vm, 0);
            if (is_falsy(condition)) {
                vm->ip += offset;
            }
            break;
        }
        
        case OP_JUMP_IF_TRUE: {
            uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            value_t condition = vm_peek(vm, 0);
            if (!is_falsy(condition)) {
                vm->ip += offset;
            }
            break;
        }
        
        case OP_SET_DEBUG_LOCATION: {
            uint16_t constant_index = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            // Read line and column bytes
            int line = *vm->ip++;
            int column = *vm->ip++;
            
            // Get the source text from the constant
            value_t source_value = function->constants[constant_index];
            if (source_value.type == VAL_STRING) {
                // Clean up previous debug location
                debug_location_free(vm->current_debug);
                
                // Create new debug location
                vm->current_debug = debug_location_create(
                    line,
                    column,
                    source_value.as.string
                );
            }
            break;
        }
        
        case OP_CLEAR_DEBUG_LOCATION: {
            debug_location_free(vm->current_debug);
            vm->current_debug = NULL;
            break;
        }

        case OP_HALT:
            vm->frame_count--;
            closure_destroy(closure);
            return VM_OK;

        default:
            printf("Unknown instruction: %s\n", opcode_name(instruction));
            vm->frame_count--;
            closure_destroy(closure);
            return VM_RUNTIME_ERROR;
        }
    }
}

vm_result vm_interpret(bitty_vm* vm, const char* source) {
    // This would compile source to bytecode and execute
    // For now, just a stub
    (void)vm;
    (void)source;
    printf("vm_interpret not yet implemented\n");
    return VM_COMPILE_ERROR;
}

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text) {
    debug_location* debug = malloc(sizeof(debug_location));
    if (!debug) return NULL;
    
    debug->line = line;
    debug->column = column;
    debug->source_text = source_text;  // Not owned, just a reference
    return debug;
}

debug_location* debug_location_copy(const debug_location* debug) {
    if (!debug) return NULL;
    
    return debug_location_create(debug->line, debug->column, debug->source_text);
}

void debug_location_free(debug_location* debug) {
    if (debug) {
        free(debug);
    }
}

// Debug utilities

// Helper function to get a specific line from source code (from parser.c)
static const char* get_source_line(const char* source, int line_number, size_t* line_length) {
    const char* current = source;
    int current_line = 1;
    const char* line_start = source;

    // Find the start of the target line
    while (*current && current_line < line_number) {
        if (*current == '\n') {
            current_line++;
            line_start = current + 1;
        }
        current++;
    }

    if (current_line != line_number) {
        *line_length = 0;
        return NULL;
    }

    // Find the end of the line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    *line_length = line_end - line_start;
    return line_start;
}

// Find debug info entry for a given bytecode offset
void* vm_get_debug_info_at(function_t* function, size_t bytecode_offset) {
    if (!function || !function->debug) {
        return NULL;
    }

    debug_info* debug = (debug_info*)function->debug;

    // Find the closest debug info entry <= bytecode_offset
    debug_info_entry* best_entry = NULL;
    for (size_t i = 0; i < debug->count; i++) {
        if (debug->entries[i].bytecode_offset <= bytecode_offset) {
            best_entry = &debug->entries[i];
        } else {
            break; // Entries should be in order
        }
    }

    return best_entry ? debug : NULL;
}

// Print enhanced runtime error with instruction location
void vm_runtime_error_with_debug(bitty_vm* vm, const char* message) {
    if (vm->frame_count == 0) {
        printf("Runtime error: %s\n", message);
        return;
    }

    // Get the current function from the top call frame
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    function_t* function = frame->closure->function;

    // Use the stored current instruction pointer
    size_t instruction_offset = vm->current_instruction - vm->bytecode;
    
    // Show the instruction that failed
    opcode failed_op = (opcode)*vm->current_instruction;
    printf("Runtime error: %s\n", message);
    printf("    at instruction %04zu: %s\n", instruction_offset, opcode_name(failed_op));

    // If we have debug info with source code, try to show it
    if (function->debug) {
        debug_info* debug = (debug_info*)function->debug;
        
        if (debug->count > 0 && debug->source_code) {
            // Find the debug info for this instruction
            debug_info_entry* entry = NULL;
            for (size_t i = 0; i < debug->count; i++) {
                if (debug->entries[i].bytecode_offset <= instruction_offset) {
                    entry = &debug->entries[i];
                } else {
                    break;
                }
            }

            if (entry) {
                size_t line_length;
                const char* line_start = get_source_line(debug->source_code, entry->line, &line_length);

                if (line_start) {
                    printf("    %.*s\n", (int)line_length, line_start);

                    // Print caret pointing to the column (adjust by -1 since columns seem to be 1-based)
                    printf("    ");
                    int caret_pos = entry->column > 0 ? entry->column - 1 : 0;
                    for (int i = 0; i < caret_pos; i++) {
                        printf(" ");
                    }
                    printf("^\n");
                }
            }
        }
    }
}

// Enhanced error reporting using value debug information
void vm_runtime_error_with_values(bitty_vm* vm, const char* format, const value_t* a, const value_t* b, debug_location* location) {
    // Print basic error message with value types
    printf("Runtime error: ");
    printf(format, value_type_name(a->type), b ? value_type_name(b->type) : "");
    printf("\n");
    
    // Use the best debug location available (preference order: location param, a->debug, b->debug, current_debug)
    debug_location* debug_to_use = location;
    if (!debug_to_use && a) debug_to_use = a->debug;
    if (!debug_to_use && b) debug_to_use = b->debug;
    if (!debug_to_use) debug_to_use = vm->current_debug;
    
    // If we have debug info, show source location
    if (debug_to_use && debug_to_use->source_text) {
        printf("    at line %d, column %d:\n", debug_to_use->line, debug_to_use->column);
        printf("    %s\n", debug_to_use->source_text);
        
        // Print caret pointing to the column
        printf("    ");
        int caret_pos = debug_to_use->column > 0 ? debug_to_use->column - 1 : 0;
        for (int i = 0; i < caret_pos; i++) {
            printf(" ");
        }
        printf("^\n");
    }
}

// Helper function to get value type name for error messages
const char* value_type_name(value_type type) {
    switch (type) {
        case VAL_NULL: return "null";
        case VAL_UNDEFINED: return "undefined";
        case VAL_BOOLEAN: return "boolean";
        case VAL_INT32: return "int32";
        case VAL_BIGINT: return "bigint";
        case VAL_NUMBER: return "number";
        case VAL_STRING: return "string";
        case VAL_ARRAY: return "array";
        case VAL_OBJECT: return "object";
        case VAL_FUNCTION: return "function";
        case VAL_CLOSURE: return "closure";
        case VAL_BUILTIN: return "builtin";
        default: return "unknown";
    }
}
