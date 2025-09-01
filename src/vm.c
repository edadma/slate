#include "vm.h"
#include "memory.h"
#include "opcodes/opcodes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtins.h"
#include "codegen.h" // For debug_info functions
#include "datetime.h" // For date/time functions

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
slate_vm* vm_create(void) {
    slate_vm* vm = malloc(sizeof(slate_vm));
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

    // Create function table - stores all defined functions with reference counting
    vm->functions = da_new(sizeof(function_t*)); // Store pointers to functions
    if (!vm->functions) {
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

slate_vm* vm_create_with_args(int argc, char** argv) {
    slate_vm* vm = vm_create();
    if (vm) {
        vm->argc = argc;
        vm->argv = argv;
    }
    return vm;
}

void vm_destroy(slate_vm* vm) {
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
    
    // Release function table (functions handle their own ref counting)
    da_release(&vm->functions);

    // Release result register
    free_value(vm->result);

    // Clean up debug location
    debug_location_free(vm->current_debug);

    free(vm);
}

void vm_reset(slate_vm* vm) {
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

// Stack operations
void vm_push(slate_vm* vm, value_t value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(slate_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(slate_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }


// Core VM execution function - executes a function with the given closure
// Assumes the VM is already set up with proper stack state and call frame

// Helper function to call functions from C code (for array methods, etc.)
value_t vm_call_function(slate_vm* vm, value_t callable, int arg_count, value_t* args) {
    if (callable.type == VAL_NATIVE) {
        // Native functions are easy - just call them directly
        native_t func = (native_t)callable.as.native;
        return func(vm, arg_count, args);
    }
    
    if (callable.type != VAL_CLOSURE && callable.type != VAL_FUNCTION) {
        // Not callable - return undefined
        return make_undefined();
    }
    
    // For user-defined functions, be flexible with argument count
    // If function expects fewer arguments than provided, just pass the first N arguments
    function_t* func = NULL;
    if (callable.type == VAL_CLOSURE) {
        func = callable.as.closure->function;
    } else if (callable.type == VAL_FUNCTION) {
        func = callable.as.function;
    }
    
    int actual_arg_count = (arg_count > func->parameter_count) ? func->parameter_count : arg_count;
           
    // Only fail if we don't have enough arguments
    if (arg_count < func->parameter_count) {
        return make_undefined();
    }
    
    // Use the real VM execution infrastructure for proper closure execution
    closure_t* closure = NULL;
    int created_closure = 0;
    
    if (callable.type == VAL_CLOSURE) {
        closure = callable.as.closure;
    } else if (callable.type == VAL_FUNCTION) {
        closure = closure_create(func);
        created_closure = 1;
    }
    
    // Set up call frame on the current VM
    if (vm->frame_count >= vm->frame_capacity) {
        if (created_closure) closure_destroy(closure);
        return make_undefined();
    }
    
    // Save current VM state
    size_t saved_stack_size = vm->stack_top - vm->stack;
    uint8_t* saved_ip = vm->ip;
    uint8_t* saved_bytecode = vm->bytecode;
    size_t saved_frame_count = vm->frame_count;
    
    // Push arguments onto VM stack
    for (int i = 0; i < actual_arg_count; i++) {
        vm_push(vm, args[i]);
    }
    
    // Set up call frame
    call_frame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = saved_ip;
    frame->slots = vm->stack_top - actual_arg_count;
    
    // Switch execution context to function
    vm->ip = func->bytecode;
    vm->bytecode = func->bytecode;
    
    // Execute the function using our core execution loop
    vm_result result = vm_run(vm);
    
    value_t return_value = make_undefined();
    if (result == VM_OK) {
        // Get return value from VM result or stack
        if (vm->stack_top > vm->stack + saved_stack_size) {
            return_value = vm_pop(vm);
        } else {
            return_value = vm->result;
        }
    }
    
    // Restore VM state
    vm->stack_top = vm->stack + saved_stack_size;
    vm->ip = saved_ip;
    vm->bytecode = saved_bytecode;
    vm->frame_count = saved_frame_count;
    
    // Clean up
    if (created_closure) {
        closure_destroy(closure);
    }
    
    return return_value;
}

// Value creation functions with debug info

// Forward declarations
ds_string value_to_string_representation(slate_vm* vm, value_t value);
static ds_string display_value_to_string(slate_vm* vm, value_t value);

// Context for object property iteration
struct object_string_context {
    ds_string* result_ptr;
    int count;
    slate_vm* vm;
};

// Callback function for object property iteration
static void object_property_to_string_callback(const char* key, void* data, size_t size, void* ctx) {
    struct object_string_context* context = (struct object_string_context*)ctx;
    ds_string* result_ptr = context->result_ptr;
    slate_vm* vm = context->vm;

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
        ds_string val_str = display_value_to_string(vm, *val);
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
ds_string value_to_string_representation(slate_vm* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING:
        return ds_retain(value.as.string);
    case VAL_INT32: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", value.as.int32);
        return ds_new(buffer);
    }
    case VAL_BIGINT: {
        char* str = di_to_string(value.as.bigint, 10);
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
                    ds_string element_str = display_value_to_string(vm, *element);
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
            struct object_string_context context = {&result, 0, vm};

            // Iterate through properties using do_foreach_property
            do_foreach_property(value.as.object, object_property_to_string_callback, &context);
        }
        ds_string bracket = ds_new("}");
        ds_string temp = ds_concat(result, bracket);
        ds_release(&result);
        ds_release(&bracket);
        return temp;
    }
    case VAL_CLASS: {
        if (value.as.class) {
            ds_string prefix = ds_new("<class ");
            ds_string name = ds_new(value.as.class->name ? value.as.class->name : "anonymous");
            ds_string suffix = ds_new(">");

            ds_string temp1 = ds_concat(prefix, name);
            ds_string result = ds_concat(temp1, suffix);

            ds_release(&prefix);
            ds_release(&name);
            ds_release(&suffix);
            ds_release(&temp1);
            return result;
        } else {
            return ds_new("<null class>");
        }
    }
    case VAL_RANGE: {
        if (!value.as.range) {
            return ds_new("{null range}");
        }

        ds_string start_str = value_to_string_representation(vm, value.as.range->start);
        ds_string range_op = ds_new(value.as.range->exclusive ? "..<" : "..");
        ds_string end_str = value_to_string_representation(vm, value.as.range->end);

        // Concatenate: start + range_op + end
        ds_string temp1 = ds_concat(start_str, range_op);
        ds_string result = ds_concat(temp1, end_str);

        // Clean up
        ds_release(&start_str);
        ds_release(&range_op);
        ds_release(&end_str);
        ds_release(&temp1);

        return result;
    }
    case VAL_ITERATOR: {
        if (!value.as.iterator) {
            return ds_new("{null iterator}");
        }

        if (value.as.iterator->type == ITER_ARRAY) {
            return ds_new("{Array Iterator}");
        } else if (value.as.iterator->type == ITER_RANGE) {
            return ds_new("{Range Iterator}");
        } else {
            return ds_new("{Unknown Iterator}");
        }
    }
    case VAL_BOUND_METHOD: {
        if (!value.as.bound_method) {
            return ds_new("{null bound method}");
        }
        return ds_new("{Bound Method}");
    }
    case VAL_FUNCTION:
        return ds_new("{Function}");
    case VAL_CLOSURE:
        return ds_new("{Closure}");
    case VAL_LOCAL_DATE: {
        if (value.as.local_date) {
            char* str = local_date_to_string(vm, value.as.local_date);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalDate>");
    }
    case VAL_LOCAL_TIME: {
        if (value.as.local_time) {
            char* str = local_time_to_string(vm, value.as.local_time);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalTime>");
    }
    case VAL_LOCAL_DATETIME: {
        if (value.as.local_datetime) {
            char* str = local_datetime_to_string(vm, value.as.local_datetime);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalDateTime>");
    }
    case VAL_ZONED_DATETIME:
        return ds_new("<ZonedDateTime>");  // TODO: implement string conversion
    case VAL_INSTANT:
        return ds_new("<Instant>");  // TODO: implement string conversion
    case VAL_DURATION:
        return ds_new("<Duration>");  // TODO: implement string conversion
    case VAL_PERIOD:
        return ds_new("<Period>");  // TODO: implement string conversion
    default:
        return ds_new("{Unknown}");
    }
}

// Helper function to convert values to display string (with quotes for strings, for use inside aggregates)
static ds_string display_value_to_string(slate_vm* vm, value_t value) {
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
        return value_to_string_representation(vm, value);
    }
}

// Print function specifically for the print() builtin - shows strings without quotes
void print_for_builtin(slate_vm* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING:
        // Print strings without quotes for direct printing
        printf("%s", value.as.string ? value.as.string : "");
        break;
    default: {
        // For aggregates and other types, use the string representation and print it
        ds_string str = value_to_string_representation(vm, value);
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
        return di_is_zero(value.as.bigint);
    case VAL_NUMBER:
        return value.as.number == 0.0;
    case VAL_STRING:
        return value.as.string == NULL || strlen(value.as.string) == 0;
    case VAL_BUFFER:
        return value.as.buffer == NULL || db_size(value.as.buffer) == 0;
    case VAL_BUFFER_BUILDER:
        return value.as.builder == NULL;
    case VAL_BUFFER_READER:
        return value.as.reader == NULL;
    default:
        return 0;
    }
}

int is_truthy(value_t value) {
    return !is_falsy(value);
}

int is_number(value_t value) { return value.type == VAL_INT32 || value.type == VAL_BIGINT || value.type == VAL_NUMBER; }

int values_equal(value_t a, value_t b) {
    // Handle cross-type numeric comparisons
    if (is_number(a) && is_number(b)) {
        // Convert to common type for comparison
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            return a.as.int32 == b.as.int32;
        } else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            return di_eq(a.as.bigint, b.as.bigint);
        } else if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
            return a.as.number == b.as.number;
        }
        // Cross-type comparisons - convert to common type
        // For now, convert to double for simplicity
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                             : a.as.number;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                             : b.as.number;
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
    case VAL_CLASS:
        return a.as.class == b.as.class; // Class identity comparison
    case VAL_BUFFER:
        if (a.as.buffer == b.as.buffer)
            return 1; // Same buffer reference
        if (a.as.buffer == NULL || b.as.buffer == NULL)
            return 0;
        return db_equals(a.as.buffer, b.as.buffer);
    case VAL_BUFFER_BUILDER:
        return a.as.builder == b.as.builder;
    case VAL_BUFFER_READER:
        return a.as.reader == b.as.reader;
    case VAL_FUNCTION:
        return a.as.function == b.as.function;
    case VAL_CLOSURE:
        return a.as.closure == b.as.closure;
    case VAL_NATIVE:
        return a.as.native == b.as.native;
    case VAL_LOCAL_DATE:
        if (a.as.local_date == b.as.local_date)
            return 1; // Same reference
        if (a.as.local_date == NULL || b.as.local_date == NULL)
            return 0;
        return local_date_equals(a.as.local_date, b.as.local_date);
    case VAL_LOCAL_TIME:
        if (a.as.local_time == b.as.local_time)
            return 1; // Same reference
        if (a.as.local_time == NULL || b.as.local_time == NULL)
            return 0;
        return local_time_equals(a.as.local_time, b.as.local_time);
    case VAL_LOCAL_DATETIME:
    case VAL_ZONED_DATETIME:
    case VAL_INSTANT:
    case VAL_DURATION:
    case VAL_PERIOD:
        // For now, use reference equality for complex types
        return a.as.local_datetime == b.as.local_datetime;
    default:
        return 0;
    }
}

void print_value(slate_vm* vm, value_t value) {
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
        char* str = di_to_string(value.as.bigint, 10);
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
    case VAL_STRING_BUILDER: {
        printf("<StringBuilder>");
        break;
    }
    case VAL_ARRAY: {
        printf("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0)
                    printf(", ");
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    print_value(vm, *element);
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
    case VAL_CLASS: {
        if (value.as.class) {
            printf("<class %s>", value.as.class->name ? value.as.class->name : "anonymous");
        } else {
            printf("<null class>");
        }
        break;
    }
    case VAL_FUNCTION:
        printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
        break;
    case VAL_CLOSURE:
        printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
        break;
    case VAL_NATIVE:
        printf("<builtin function>");
        break;
    case VAL_RANGE: {
        if (!value.as.range) {
            printf("<null range>");
        } else {
            print_value(vm, value.as.range->start);
            printf(value.as.range->exclusive ? "..<" : "..");
            print_value(vm, value.as.range->end);
        }
        break;
    }
    case VAL_ITERATOR: {
        if (!value.as.iterator) {
            printf("<null iterator>");
        } else if (value.as.iterator->type == ITER_ARRAY) {
            printf("<array iterator>");
        } else if (value.as.iterator->type == ITER_RANGE) {
            printf("<range iterator>");
        } else {
            printf("<unknown iterator>");
        }
        break;
    }
    case VAL_BUFFER: {
        if (!value.as.buffer) {
            printf("<null buffer>");
        } else {
            printf("<buffer size=%zu>", db_size(value.as.buffer));
        }
        break;
    }
    case VAL_BUFFER_BUILDER: {
        if (!value.as.builder) {
            printf("<null buffer builder>");
        } else {
            printf("<buffer builder>");
        }
        break;
    }
    case VAL_BUFFER_READER: {
        if (!value.as.reader) {
            printf("<null buffer reader>");
        } else {
            printf("<buffer reader pos=%zu>", db_reader_position(value.as.reader));
        }
        break;
    }
    case VAL_BOUND_METHOD: {
        if (!value.as.bound_method) {
            printf("<null bound method>");
        } else {
            printf("<bound method>");
        }
        break;
    }
    case VAL_LOCAL_DATE: {
        if (value.as.local_date) {
            char* str = local_date_to_string(vm, value.as.local_date);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDate>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_TIME: {
        if (value.as.local_time) {
            char* str = local_time_to_string(vm, value.as.local_time);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_DATETIME: {
        if (value.as.local_datetime) {
            char* str = local_datetime_to_string(vm, value.as.local_datetime);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDateTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_ZONED_DATETIME: {
        printf("<ZonedDateTime>");  // TODO: implement string conversion
        break;
    }
    case VAL_INSTANT: {
        printf("<Instant>");  // TODO: implement string conversion
        break;
    }
    case VAL_DURATION: {
        printf("<Duration>");  // TODO: implement string conversion
        break;
    }
    case VAL_PERIOD: {
        printf("<Period>");  // TODO: implement string conversion
        break;
    }
    }
}

double value_to_double(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return (double)value.as.int32;
    case VAL_BIGINT:
        return di_to_double(value.as.bigint);
    case VAL_NUMBER:
        return value.as.number;
    default:
        runtime_error("Cannot convert %s to number", value_type_name(value.type));
        return 0.0; // Never reached, but keeps compiler happy
    }
}

bool is_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return true;
    case VAL_BIGINT:
        // Check if BigInt fits in int32 range
        int32_t dummy;
        return di_to_int32(value.as.bigint, &dummy);
    case VAL_NUMBER:
        // Check if the number is a whole number that fits in int range
        return value.as.number == floor(value.as.number) && value.as.number >= INT_MIN && value.as.number <= INT_MAX;
    default:
        return false;
    }
}

int value_to_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return value.as.int32;
    case VAL_BIGINT:
        // Convert BigInt to int, checking for overflow
        int32_t result;
        if (di_to_int32(value.as.bigint, &result)) {
            return result;
        } else {
            char* str = di_to_string(value.as.bigint, 10);
            runtime_error("BigInt value %s too large for integer", str);
            free(str);
            return 0; // Never reached
        }
    case VAL_NUMBER:
        // Check if it's a valid integer
        if (value.as.number == floor(value.as.number) && value.as.number >= INT_MIN && value.as.number <= INT_MAX) {
            return (int)value.as.number;
        } else {
            runtime_error("Number %g is not a valid integer", value.as.number);
            return 0; // Never reached
        }
    default:
        runtime_error("Cannot convert %s to integer", value_type_name(value.type));
        return 0; // Never reached, but keeps compiler happy
    }
}


// Constant pool management
size_t vm_add_constant(slate_vm* vm, value_t value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(slate_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
}

// Function table management
size_t vm_add_function(slate_vm* vm, function_t* function) {
    size_t index = vm->functions->length;
    DA_PUSH(vm->functions, function);
    return index;
}

function_t* vm_get_function(slate_vm* vm, size_t index) {
    assert(index < vm->functions->length);
    return DA_AT(vm->functions, index, function_t*);
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
    case OP_NULL_COALESCE:
        return "NULL_COALESCE";
    case OP_IN:
        return "IN";
    case OP_INSTANCEOF:
        return "INSTANCEOF";
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
    case OP_BUILD_RANGE:
        return "BUILD_RANGE";
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
    case OP_POP_N:
        return "POP_N";
    case OP_POP_N_PRESERVE_TOP:
        return "POP_N_PRESERVE_TOP";
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

// Core VM execution loop - runs until completion
// Assumes VM is already set up with proper call frames and stack
vm_result vm_run(slate_vm* vm) {
    if (!vm)
        return VM_RUNTIME_ERROR;

    // Main execution loop
    for (;;) {
        vm->current_instruction = vm->ip; // Store instruction start for error reporting
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            vm_result result = op_push_constant(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_NULL: {
            vm_result result = op_push_null(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_UNDEFINED: {
            vm_result result = op_push_undefined(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_TRUE: {
            vm_result result = op_push_true(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_FALSE: {
            vm_result result = op_push_false(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POP: {
            vm_result result = op_pop(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DUP: {
            vm_result result = op_dup(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_RESULT: {
            vm_result result = op_set_result(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_ADD: {
            vm_result result = op_add(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SUBTRACT: {
            vm_result result = op_subtract(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MULTIPLY: {
            vm_result result = op_multiply(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DIVIDE: {
            vm_result result = op_divide(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NEGATE: {
            vm_result result = op_negate(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MOD: {
            vm_result result = op_mod(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POWER: {
            vm_result result = op_power(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_EQUAL: {
            vm_result result = op_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NOT_EQUAL: {
            vm_result result = op_not_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_AND: {
            vm_result result = op_and(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_OR: {
            vm_result result = op_or(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NOT: {
            vm_result result = op_not(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LESS: {
            vm_result result = op_less(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GREATER: {
            vm_result result = op_greater(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LESS_EQUAL: {
            vm_result result = op_less_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GREATER_EQUAL: {
            vm_result result = op_greater_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_RETURN: {
            vm_result result = op_return(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_LOCAL: {
            vm_result result = op_get_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_LOCAL: {
            vm_result result = op_set_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_GLOBAL: {
            vm_result result = op_get_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DEFINE_GLOBAL: {
            vm_result result = op_define_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_GLOBAL: {
            vm_result result = op_set_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_PROPERTY: {
            vm_result result = op_get_property(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CALL: {
            vm_result result = op_call(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CLOSURE: {
            vm_result result = op_closure(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_ARRAY: {
            vm_result result = op_build_array(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_OBJECT: {
            vm_result result = op_build_object(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_DEBUG_LOCATION: {
            vm_result result = op_set_debug_location(vm);
            if (result != VM_OK) return result;
            break;
        }
        
        case OP_CLEAR_DEBUG_LOCATION: {
            vm_result result = op_clear_debug_location(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_JUMP: {
            vm_result result = op_jump(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_JUMP_IF_FALSE: {
            vm_result result = op_jump_if_false(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_HALT: {
            vm_result result = op_halt(vm);
            return result;
        }

        // Missing opcodes that were causing test failures
        case OP_BITWISE_OR: {
            vm_result result = op_bitwise_or(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BITWISE_XOR: {
            vm_result result = op_bitwise_xor(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BITWISE_NOT: {
            vm_result result = op_bitwise_not(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LEFT_SHIFT: {
            vm_result result = op_left_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_RIGHT_SHIFT: {
            vm_result result = op_right_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LOGICAL_RIGHT_SHIFT: {
            vm_result result = op_logical_right_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_FLOOR_DIV: {
            vm_result result = op_floor_div(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_INCREMENT: {
            vm_result result = op_increment(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DECREMENT: {
            vm_result result = op_decrement(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_IN: {
            vm_result result = op_in(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CALL_METHOD: {
            vm_result result = op_call_method(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POP_N_PRESERVE_TOP: {
            vm_result result = op_pop_n_preserve_top(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_RANGE: {
            vm_result result = op_build_range(vm);
            if (result != VM_OK) return result;
            break;
        }

        default:
            printf("DEBUG: Unimplemented opcode in vm_run: %d\n", instruction);
            return VM_RUNTIME_ERROR;
        }
    }
}

// VM execution with setup - clears stack and sets up initial call frame
vm_result vm_execute(slate_vm* vm, function_t* function) {
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

    // Run the VM using the new core execution function
    vm_result result = vm_run(vm);
    
    // Clean up closure if execution completed normally
    if (result == VM_OK) {
        closure_destroy(closure);
    }
    
    return result;
}
vm_result vm_interpret(slate_vm* vm, const char* source) {
    // This would compile source to bytecode and execute
    // For now, just a stub
    (void)vm;
    (void)source;
    printf("vm_interpret not yet implemented\n");
    return VM_COMPILE_ERROR;
}

// Iterator creation and management functions
iterator_t* create_array_iterator(da_array array) {
    iterator_t* iter = malloc(sizeof(iterator_t));
    if (!iter)
        return NULL;

    iter->ref_count = 1; // Initialize reference count
    iter->type = ITER_ARRAY;
    iter->data.array_iter.array = da_retain(array);
    iter->data.array_iter.index = 0;

    return iter;
}

iterator_t* create_range_iterator(value_t start, value_t end, int exclusive) {
    iterator_t* iter = malloc(sizeof(iterator_t));
    if (!iter)
        return NULL;

    iter->ref_count = 1; // Initialize reference count
    iter->type = ITER_RANGE;
    iter->data.range_iter.current = vm_retain(start);
    iter->data.range_iter.end = vm_retain(end);
    iter->data.range_iter.exclusive = exclusive;
    iter->data.range_iter.finished = 0;

    return iter;
}

int iterator_has_next(iterator_t* iter) {
    if (!iter)
        return 0;

    switch (iter->type) {
    case ITER_ARRAY:
        return iter->data.array_iter.index < da_length(iter->data.array_iter.array);
    case ITER_RANGE: {
        if (iter->data.range_iter.finished)
            return 0;

        // For now, only support integer ranges
        if (iter->data.range_iter.current.type != VAL_INT32 || iter->data.range_iter.end.type != VAL_INT32) {
            return 0;
        }

        int32_t current = iter->data.range_iter.current.as.int32;
        int32_t end = iter->data.range_iter.end.as.int32;

        if (iter->data.range_iter.exclusive) {
            return current < end;
        } else {
            return current <= end;
        }
    }
    default:
        return 0;
    }
}

value_t iterator_next(iterator_t* iter) {
    if (!iter || !iterator_has_next(iter)) {
        return make_null();
    }

    switch (iter->type) {
    case ITER_ARRAY: {
        value_t* element = (value_t*)da_get(iter->data.array_iter.array, iter->data.array_iter.index);
        iter->data.array_iter.index++;
        return element ? vm_retain(*element) : make_null();
    }
    case ITER_RANGE: {
        // Return current value and increment
        value_t current = vm_retain(iter->data.range_iter.current);

        // Increment current (for now, only support integers)
        if (iter->data.range_iter.current.type == VAL_INT32) {
            iter->data.range_iter.current.as.int32++;
        } else {
            iter->data.range_iter.finished = 1;
        }

        return current;
    }
    default:
        return make_null();
    }
}

// Bound method reference counting functions
bound_method_t* bound_method_retain(bound_method_t* method) {
    if (!method)
        return method;
    method->ref_count++;
    return method;
}

void bound_method_release(bound_method_t* method) {
    if (!method)
        return;

    method->ref_count--;
    if (method->ref_count == 0) {
        // Clean up bound method data
        vm_release(method->receiver);

        // Free the bound method itself
        free(method);
    }
}

// Class reference counting functions

// Range reference counting functions
range_t* range_retain(range_t* range) {
    if (!range)
        return range;
    range->ref_count++;
    return range;
}

void range_release(range_t* range) {
    if (!range)
        return;

    range->ref_count--;
    if (range->ref_count == 0) {
        // Clean up range data
        vm_release(range->start);
        vm_release(range->end);

        // Free the range itself
        free(range);
    }
}

// Iterator reference counting functions
iterator_t* iterator_retain(iterator_t* iter) {
    if (!iter)
        return iter;
    iter->ref_count++;
    return iter;
}

void iterator_release(iterator_t* iter) {
    if (!iter)
        return;

    iter->ref_count--;
    if (iter->ref_count == 0) {
        // Clean up iterator-specific data
        if (iter->type == ITER_ARRAY) {
            da_release(&iter->data.array_iter.array);
        } else if (iter->type == ITER_RANGE) {
            vm_release(iter->data.range_iter.current);
            vm_release(iter->data.range_iter.end);
        }

        // Free the iterator itself
        free(iter);
    }
}

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text) {
    debug_location* debug = malloc(sizeof(debug_location));
    if (!debug)
        return NULL;

    debug->line = line;
    debug->column = column;
    debug->source_text = source_text; // Not owned, just a reference
    return debug;
}

debug_location* debug_location_copy(const debug_location* debug) {
    if (!debug)
        return NULL;

    return debug_location_create(debug->line, debug->column, debug->source_text);
}

void debug_location_free(debug_location* debug) {
    if (debug && (uintptr_t)debug > 0x1000) {  // Basic sanity check - valid pointers are usually much higher
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
void vm_runtime_error_with_debug(slate_vm* vm, const char* message) {
    if (vm->frame_count == 0) {
        printf("Runtime error: %s\n", message);
        exit(1);
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
    
    // Terminate program execution
    exit(1);
}

// Enhanced error reporting using value debug information
void vm_runtime_error_with_values(slate_vm* vm, const char* format, const value_t* a, const value_t* b,
                                  debug_location* location) {
    // Print basic error message with value types
    printf("Runtime error: ");
    printf(format, value_type_name(a->type), b ? value_type_name(b->type) : "");
    printf("\n");

    // Use the best debug location available (preference order: location param, a->debug, b->debug, current_debug)
    debug_location* debug_to_use = location;
    if (!debug_to_use && a)
        debug_to_use = a->debug;
    if (!debug_to_use && b)
        debug_to_use = b->debug;
    if (!debug_to_use)
        debug_to_use = vm->current_debug;

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
    case VAL_NULL:
        return "null";
    case VAL_UNDEFINED:
        return "undefined";
    case VAL_BOOLEAN:
        return "boolean";
    case VAL_INT32:
        return "int32";
    case VAL_BIGINT:
        return "bigint";
    case VAL_NUMBER:
        return "number";
    case VAL_STRING:
        return "string";
    case VAL_STRING_BUILDER:
        return "string_builder";
    case VAL_ARRAY:
        return "array";
    case VAL_OBJECT:
        return "object";
    case VAL_CLASS:
        return "class";
    case VAL_RANGE:
        return "range";
    case VAL_ITERATOR:
        return "iterator";
    case VAL_BUFFER:
        return "buffer";
    case VAL_BUFFER_BUILDER:
        return "buffer_builder";
    case VAL_BUFFER_READER:
        return "buffer_reader";
    case VAL_FUNCTION:
        return "function";
    case VAL_CLOSURE:
        return "closure";
    case VAL_NATIVE:
        return "builtin";
    case VAL_BOUND_METHOD:
        return "bound_method";
    case VAL_LOCAL_DATE:
        return "LocalDate";
    case VAL_LOCAL_TIME:
        return "LocalTime";
    case VAL_LOCAL_DATETIME:
        return "LocalDateTime";
    case VAL_ZONED_DATETIME:
        return "ZonedDateTime";
    case VAL_INSTANT:
        return "Instant";
    case VAL_DURATION:
        return "Duration";
    case VAL_PERIOD:
        return "Period";
    default:
        return "unknown";
    }
}
