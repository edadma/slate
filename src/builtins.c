#include "builtins.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

// Static random initialization flag
static int random_initialized = 0;

// Runtime error handling - exits with non-zero code for now, will throw exception later
void runtime_error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    
    fprintf(stderr, "Runtime error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    exit(1);  // Non-zero exit code
}

// Register a built-in function in the VM's global namespace
void register_builtin(bitty_vm* vm, const char* name, builtin_func_t func, int min_args, int max_args) {
    // Create a built-in function value
    value_t builtin_val = make_builtin((void*)func);
    
    // Store in the VM's global namespace
    do_set(vm->globals, name, &builtin_val, sizeof(value_t));
}


// Initialize all built-in functions
void builtins_init(bitty_vm* vm) {
    // Initialize random seed once
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }
    
    // Register all built-ins
    register_builtin(vm, "print", builtin_print, 1, 1);
    register_builtin(vm, "type", builtin_type, 1, 1);
    register_builtin(vm, "abs", builtin_abs, 1, 1);
    register_builtin(vm, "sqrt", builtin_sqrt, 1, 1);
    register_builtin(vm, "floor", builtin_floor, 1, 1);
    register_builtin(vm, "ceil", builtin_ceil, 1, 1);
    register_builtin(vm, "round", builtin_round, 1, 1);
    register_builtin(vm, "min", builtin_min, 2, 2);
    register_builtin(vm, "max", builtin_max, 2, 2);
    register_builtin(vm, "random", builtin_random, 0, 0);
    register_builtin(vm, "sin", builtin_sin, 1, 1);
    register_builtin(vm, "cos", builtin_cos, 1, 1);
    register_builtin(vm, "tan", builtin_tan, 1, 1);
    register_builtin(vm, "input", builtin_input, 0, 1);
    register_builtin(vm, "parse_int", builtin_parse_int, 1, 1);
    register_builtin(vm, "parse_number", builtin_parse_number, 1, 1);
    register_builtin(vm, "args", builtin_args, 0, 0);
    
    // Iterator functions
    register_builtin(vm, "iterator", builtin_iterator, 1, 1);
    register_builtin(vm, "hasNext", builtin_has_next, 1, 1);
    register_builtin(vm, "next", builtin_next, 1, 1);
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("print() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Use the specialized print function for builtins
    print_for_builtin(arg);
    printf("\n");
    
    return make_undefined(); // print returns void
}

// type(value) - Get type name as string
value_t builtin_type(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("type() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    const char* type_name;
    
    switch (arg.type) {
    case VAL_INT32:
        type_name = "int32";
        break;
    case VAL_BIGINT:
        type_name = "bigint";
        break;
    case VAL_NUMBER:
        type_name = "number";
        break;
    case VAL_STRING:
        type_name = "string";
        break;
    case VAL_BOOLEAN:
        type_name = "boolean";
        break;
    case VAL_NULL:
        type_name = "null";
        break;
    case VAL_UNDEFINED:
        type_name = "undefined";
        break;
    case VAL_ARRAY:
        type_name = "array";
        break;
    case VAL_OBJECT:
        type_name = "object";
        break;
    case VAL_FUNCTION:
        type_name = "function";
        break;
    case VAL_CLOSURE:
        type_name = "closure";
        break;
    case VAL_BUILTIN:
        type_name = "builtin";
        break;
    case VAL_RANGE:
        type_name = "range";
        break;
    case VAL_ITERATOR:
        type_name = "iterator";
        break;
    case VAL_BOUND_METHOD:
        type_name = "bound_method";
        break;
    default:
        type_name = "unknown";
        break;
    }
    
    return make_string(type_name);
}

// abs(number) - Absolute value
value_t builtin_abs(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("abs() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        int32_t val = arg.as.int32;
        if (val == INT32_MIN) {
            // abs(INT32_MIN) overflows to BigInt
            db_bigint big = db_from_int64(-(int64_t)INT32_MIN);
            return make_bigint(big);
        } else {
            return make_int32(val < 0 ? -val : val);
        }
    } else if (arg.type == VAL_BIGINT) {
        db_bigint result = db_abs(arg.as.bigint);
        return make_bigint(result);
    } else if (arg.type == VAL_NUMBER) {
        return make_number(fabs(arg.as.number));
    } else {
        runtime_error("abs() requires a number argument");
    }
}

// sqrt(number) - Square root
value_t builtin_sqrt(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sqrt() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Convert to double for sqrt calculation
    double val;
    if (arg.type == VAL_INT32) {
        val = (double)arg.as.int32;
    } else if (arg.type == VAL_BIGINT) {
        val = db_to_double(arg.as.bigint);
    } else if (arg.type == VAL_NUMBER) {
        val = arg.as.number;
    } else {
        runtime_error("sqrt() requires a number argument");
    }
    
    if (val < 0) {
        runtime_error("sqrt() of negative number");
    }
    
    return make_number(sqrt(val));
}

// floor(number) - Floor function
value_t builtin_floor(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("floor() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "floored"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "floored"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = floor(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("floor() requires a number argument");
    }
}

// ceil(number) - Ceiling function
value_t builtin_ceil(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("ceil() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "ceiled"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "ceiled"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = ceil(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("ceil() requires a number argument");
    }
}

// round(number) - Round to nearest integer
value_t builtin_round(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("round() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "rounded"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "rounded"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = round(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("round() requires a number argument");
    }
}

// min(a, b) - Minimum of two values
value_t builtin_min(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("min() takes exactly 2 arguments (%d given)", arg_count);
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    // Check if both are numeric types
    if (!((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
          (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER))) {
        runtime_error("min() requires number arguments");
    }
    
    // Convert both to double for comparison
    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                   a.as.number;
    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                   b.as.number;
    
    // Return the smaller value with its original type
    return (a_val < b_val) ? a : b;
}

// max(a, b) - Maximum of two values
value_t builtin_max(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("max() takes exactly 2 arguments (%d given)", arg_count);
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    // Check if both are numeric types
    if (!((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
          (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER))) {
        runtime_error("max() requires number arguments");
    }
    
    // Convert both to double for comparison
    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32 :
                   (a.type == VAL_BIGINT) ? db_to_double(a.as.bigint) :
                   a.as.number;
    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32 :
                   (b.type == VAL_BIGINT) ? db_to_double(b.as.bigint) :
                   b.as.number;
    
    // Return the larger value with its original type
    return (a_val > b_val) ? a : b;
}

// random() - Random number 0.0 to 1.0
value_t builtin_random(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error("random() takes no arguments (%d given)", arg_count);
    }
    
    return make_number((double)rand() / RAND_MAX);
}

// sin(number) - Sine function (radians)
value_t builtin_sin(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sin() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Convert to double for sin calculation
    double val;
    if (arg.type == VAL_INT32) {
        val = (double)arg.as.int32;
    } else if (arg.type == VAL_BIGINT) {
        val = db_to_double(arg.as.bigint);
    } else if (arg.type == VAL_NUMBER) {
        val = arg.as.number;
    } else {
        runtime_error("sin() requires a number argument");
    }
    
    return make_number(sin(val));
}

// cos(number) - Cosine function (radians)
value_t builtin_cos(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("cos() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Convert to double for cos calculation
    double val;
    if (arg.type == VAL_INT32) {
        val = (double)arg.as.int32;
    } else if (arg.type == VAL_BIGINT) {
        val = db_to_double(arg.as.bigint);
    } else if (arg.type == VAL_NUMBER) {
        val = arg.as.number;
    } else {
        runtime_error("cos() requires a number argument");
    }
    
    return make_number(cos(val));
}

// tan(number) - Tangent function (radians)
value_t builtin_tan(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("tan() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    // Convert to double for tan calculation
    double val;
    if (arg.type == VAL_INT32) {
        val = (double)arg.as.int32;
    } else if (arg.type == VAL_BIGINT) {
        val = db_to_double(arg.as.bigint);
    } else if (arg.type == VAL_NUMBER) {
        val = arg.as.number;
    } else {
        runtime_error("tan() requires a number argument");
    }
    
    return make_number(tan(val));
}

// input(prompt) - Read user input with optional prompt
value_t builtin_input(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count > 1) {
        runtime_error("input() takes 0 or 1 arguments (%d given)", arg_count);
    }
    
    // Print prompt if provided
    if (arg_count == 1) {
        value_t prompt = args[0];
        if (prompt.type == VAL_STRING) {
            printf("%s", prompt.as.string);
            fflush(stdout);
        } else {
            runtime_error("input() prompt must be a string");
        }
    }
    
    // Read line from stdin
    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);
    
    if (read == -1) {
        // EOF or error
        free(line);
        return make_null();
    }
    
    // Remove trailing newline if present
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }
    
    // Create string value and free the buffer
    ds_string result = ds_new(line);
    free(line);
    
    return make_string_ds(result);
}

// parse_int(string) - Convert string to integer
value_t builtin_parse_int(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("parse_int() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error("parse_int() requires a string argument");
    }
    
    char* endptr;
    const char* str = arg.as.string;
    
    // Try parsing as long long first
    long long val = strtoll(str, &endptr, 10);
    
    // Check if entire string was consumed
    if (*endptr != '\0') {
        runtime_error("'%s' is not a valid integer", str);
    }
    
    // Check if it fits in int32
    if (val >= INT32_MIN && val <= INT32_MAX) {
        return make_int32((int32_t)val);
    } else {
        // Use BigInt for large numbers
        db_bigint big = db_from_int64(val);
        return make_bigint(big);
    }
}

// parse_number(string) - Convert string to number (int or float)
value_t builtin_parse_number(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("parse_number() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error("parse_number() requires a string argument");
    }
    
    const char* str = arg.as.string;
    char* endptr;
    
    // Check if it contains a decimal point
    if (strchr(str, '.') != NULL) {
        // Parse as float
        double val = strtod(str, &endptr);
        
        if (*endptr != '\0') {
            runtime_error("'%s' is not a valid number", str);
        }
        
        return make_number(val);
    } else {
        // Parse as integer first, then convert to appropriate type
        long long val = strtoll(str, &endptr, 10);
        
        if (*endptr != '\0') {
            runtime_error("'%s' is not a valid number", str);
        }
        
        // Check if it fits in int32
        if (val >= INT32_MIN && val <= INT32_MAX) {
            return make_int32((int32_t)val);
        } else {
            // Use BigInt for large numbers
            db_bigint big = db_from_int64(val);
            return make_bigint(big);
        }
    }
}

// args() - Get command line arguments as array
value_t builtin_args(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error("args() takes no arguments (%d given)", arg_count);
    }
    
    // Create array of command line arguments
    da_array arg_array = da_new(sizeof(value_t));
    
    for (int i = 0; i < vm->argc; i++) {
        value_t arg_val = make_string(vm->argv[i]);
        da_push(arg_array, &arg_val);
    }
    
    return make_array(arg_array);
}

// iterator(collection) - Create iterator for arrays and ranges
value_t builtin_iterator(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("iterator() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t collection = args[0];
    iterator_t* iter = NULL;
    
    switch (collection.type) {
    case VAL_ARRAY:
        iter = create_array_iterator(collection.as.array);
        break;
    case VAL_RANGE:
        if (collection.as.range) {
            iter = create_range_iterator(
                collection.as.range->start,
                collection.as.range->end,
                collection.as.range->exclusive
            );
        }
        break;
    default:
        runtime_error("iterator() can only be called on arrays and ranges, not %s", value_type_name(collection.type));
    }
    
    if (!iter) {
        runtime_error("Failed to create iterator");
    }
    
    return make_iterator(iter);
}

// hasNext(iterator) - Check if iterator has more elements
value_t builtin_has_next(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("hasNext() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error("hasNext() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }
    
    int has_next = iterator_has_next(iter_val.as.iterator);
    return make_boolean(has_next);
}

// next(iterator) - Get next element from iterator
value_t builtin_next(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("next() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error("next() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }
    
    if (!iterator_has_next(iter_val.as.iterator)) {
        runtime_error("Iterator has no more elements");
    }
    
    return iterator_next(iter_val.as.iterator);
}