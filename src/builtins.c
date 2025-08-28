#include "builtins.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    exit(1); // Non-zero exit code
}

// Register a built-in function in the VM's global namespace
void register_builtin(slate_vm* vm, const char* name, native_t func, int min_args, int max_args) {
    // Create a built-in function value
    value_t builtin_val = make_native((void*)func);

    // Store in the VM's global namespace
    do_set(vm->globals, name, &builtin_val, sizeof(value_t));
}

// Initialize all built-in functions
void builtins_init(slate_vm* vm) {
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
    register_builtin(vm, "asin", builtin_asin, 1, 1);
    register_builtin(vm, "acos", builtin_acos, 1, 1);
    register_builtin(vm, "atan", builtin_atan, 1, 1);
    register_builtin(vm, "atan2", builtin_atan2, 2, 2);
    register_builtin(vm, "degrees", builtin_degrees, 1, 1);
    register_builtin(vm, "radians", builtin_radians, 1, 1);
    register_builtin(vm, "exp", builtin_exp, 1, 1);
    register_builtin(vm, "ln", builtin_ln, 1, 1);
    register_builtin(vm, "sign", builtin_sign, 1, 1);
    register_builtin(vm, "input", builtin_input, 0, 1);
    register_builtin(vm, "parse_int", builtin_parse_int, 1, 1);
    register_builtin(vm, "parse_number", builtin_parse_number, 1, 1);
    register_builtin(vm, "args", builtin_args, 0, 0);

    // Iterator functions
    register_builtin(vm, "iterator", builtin_iterator, 1, 1);
    register_builtin(vm, "hasNext", builtin_has_next, 1, 1);
    register_builtin(vm, "next", builtin_next, 1, 1);

    // Buffer functions
    register_builtin(vm, "buffer", builtin_buffer, 1, 1);
    register_builtin(vm, "buffer_from_hex", builtin_buffer_from_hex, 1, 1);
    register_builtin(vm, "buffer_slice", builtin_buffer_slice, 3, 3);
    register_builtin(vm, "buffer_concat", builtin_buffer_concat, 2, 2);
    register_builtin(vm, "buffer_to_hex", builtin_buffer_to_hex, 1, 1);

    // Buffer builder functions
    register_builtin(vm, "buffer_builder", builtin_buffer_builder, 1, 1);
    register_builtin(vm, "builder_append_uint8", builtin_builder_append_uint8, 2, 2);
    register_builtin(vm, "builder_append_uint16_le", builtin_builder_append_uint16_le, 2, 2);
    register_builtin(vm, "builder_append_uint32_le", builtin_builder_append_uint32_le, 2, 2);
    register_builtin(vm, "builder_append_cstring", builtin_builder_append_cstring, 2, 2);
    register_builtin(vm, "builder_finish", builtin_builder_finish, 1, 1);

    // Buffer reader functions
    register_builtin(vm, "buffer_reader", builtin_buffer_reader, 1, 1);
    register_builtin(vm, "reader_read_uint8", builtin_reader_read_uint8, 1, 1);
    register_builtin(vm, "reader_read_uint16_le", builtin_reader_read_uint16_le, 1, 1);
    register_builtin(vm, "reader_read_uint32_le", builtin_reader_read_uint32_le, 1, 1);
    register_builtin(vm, "reader_position", builtin_reader_position, 1, 1);
    register_builtin(vm, "reader_remaining", builtin_reader_remaining, 1, 1);

    // I/O functions
    register_builtin(vm, "read_file", builtin_read_file, 1, 1);
    register_builtin(vm, "write_file", builtin_write_file, 2, 2);
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_type(slate_vm* vm, int arg_count, value_t* args) {
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
    case VAL_NATIVE:
        type_name = "builtin";
        break;
    case VAL_RANGE:
        type_name = "range";
        break;
    case VAL_ITERATOR:
        type_name = "iterator";
        break;
    case VAL_BUFFER:
        type_name = "buffer";
        break;
    case VAL_BUFFER_BUILDER:
        type_name = "buffer_builder";
        break;
    case VAL_BUFFER_READER:
        type_name = "buffer_reader";
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
value_t builtin_abs(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("abs() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        int32_t val = arg.as.int32;
        if (val == INT32_MIN) {
            // abs(INT32_MIN) overflows to BigInt
            di_int big = di_from_int64(-(int64_t)INT32_MIN);
            return make_bigint(big);
        } else {
            return make_int32(val < 0 ? -val : val);
        }
    } else if (arg.type == VAL_BIGINT) {
        di_int result = di_abs(arg.as.bigint);
        return make_bigint(result);
    } else if (arg.type == VAL_NUMBER) {
        return make_number(fabs(arg.as.number));
    } else {
        runtime_error("abs() requires a number argument");
    }
}

// sqrt(number) - Square root
value_t builtin_sqrt(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sqrt() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sqrt() requires a number argument");
    }

    double val = value_to_double(arg);

    if (val < 0) {
        runtime_error("sqrt() of negative number");
    }

    return make_number(sqrt(val));
}

// floor(number) - Floor function
value_t builtin_floor(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_ceil(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_round(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_min(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("min() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error("min() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_double(a);
    double b_val = value_to_double(b);

    // Return the smaller value with its original type
    return (a_val < b_val) ? a : b;
}

// max(a, b) - Maximum of two values
value_t builtin_max(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("max() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error("max() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_double(a);
    double b_val = value_to_double(b);

    // Return the larger value with its original type
    return (a_val > b_val) ? a : b;
}

// random() - Random number 0.0 to 1.0
value_t builtin_random(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error("random() takes no arguments (%d given)", arg_count);
    }

    return make_number((double)rand() / RAND_MAX);
}

// sin(number) - Sine function (radians)
value_t builtin_sin(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sin() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(sin(val));
}

// cos(number) - Cosine function (radians)
value_t builtin_cos(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("cos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("cos() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(cos(val));
}

// tan(number) - Tangent function (radians)
value_t builtin_tan(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("tan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("tan() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(tan(val));
}

// exp(number) - Exponential function (e^x)
value_t builtin_exp(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("exp() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("exp() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(exp(val));
}

// ln(number) - Natural logarithm (base e)
value_t builtin_ln(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("ln() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("ln() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (ln of non-positive numbers)
    if (val <= 0) {
        runtime_error("ln() domain error: argument must be positive");
    }

    return make_number(log(val));
}

// asin(number) - Inverse sine function (returns radians)
value_t builtin_asin(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("asin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("asin() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (asin domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error("asin() domain error: argument must be in range [-1, 1]");
    }

    return make_number(asin(val));
}

// acos(number) - Inverse cosine function (returns radians)
value_t builtin_acos(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("acos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("acos() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (acos domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error("acos() domain error: argument must be in range [-1, 1]");
    }

    return make_number(acos(val));
}

// atan(number) - Inverse tangent function (returns radians)
value_t builtin_atan(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("atan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("atan() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(atan(val));
}

// atan2(y, x) - Two-argument inverse tangent function (returns radians)
value_t builtin_atan2(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("atan2() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t y = args[0];
    value_t x = args[1];

    // Check if both are numeric types
    if (!is_number(y) || !is_number(x)) {
        runtime_error("atan2() requires number arguments");
    }

    double y_val = value_to_double(y);
    double x_val = value_to_double(x);

    return make_number(atan2(y_val, x_val));
}

// degrees(radians) - Convert radians to degrees
value_t builtin_degrees(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("degrees() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("degrees() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(val * 180.0 / M_PI);
}

// radians(degrees) - Convert degrees to radians
value_t builtin_radians(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("radians() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("radians() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(val * M_PI / 180.0);
}

// sign(number) - Sign function (-1, 0, or 1)
value_t builtin_sign(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sign() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sign() requires a number argument");
    }

    double val = value_to_double(arg);

    if (val > 0.0) {
        return make_int32(1);
    } else if (val < 0.0) {
        return make_int32(-1);
    } else {
        return make_int32(0);
    }
}

// input(prompt) - Read user input with optional prompt
value_t builtin_input(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_parse_int(slate_vm* vm, int arg_count, value_t* args) {
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
        di_int big = di_from_int64(val);
        return make_bigint(big);
    }
}

// parse_number(string) - Convert string to number (int or float)
value_t builtin_parse_number(slate_vm* vm, int arg_count, value_t* args) {
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
            di_int big = di_from_int64(val);
            return make_bigint(big);
        }
    }
}

// args() - Get command line arguments as array
value_t builtin_args(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_iterator(slate_vm* vm, int arg_count, value_t* args) {
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
            iter = create_range_iterator(collection.as.range->start, collection.as.range->end,
                                         collection.as.range->exclusive);
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
value_t builtin_has_next(slate_vm* vm, int arg_count, value_t* args) {
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
value_t builtin_next(slate_vm* vm, int arg_count, value_t* args) {
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

// ========================
// BUFFER BUILTIN FUNCTIONS
// ========================

// buffer(data) - Create buffer from string or array
value_t builtin_buffer(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t data = args[0];
    db_buffer buf;

    if (data.type == VAL_STRING) {
        // Create buffer from string
        const char* str = data.as.string;
        size_t len = str ? strlen(str) : 0;
        buf = db_new_with_data(str, len);
    } else if (data.type == VAL_ARRAY) {
        // Create buffer from array of numbers
        da_array arr = data.as.array;
        size_t len = da_length(arr);

        // Convert array elements to bytes
        uint8_t* bytes = malloc(len);
        if (!bytes) {
            runtime_error("Failed to allocate memory for buffer");
        }

        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            if (!elem) {
                free(bytes);
                runtime_error("Invalid array element at index %zu", i);
            }
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0 || elem->as.int32 > 255) {
                    free(bytes);
                    runtime_error("Array element %d at index %zu is not a valid byte (0-255)", elem->as.int32, i);
                }
                bytes[i] = (uint8_t)elem->as.int32;
            } else {
                free(bytes);
                runtime_error("Array element at index %zu must be an integer, not %s", i, value_type_name(elem->type));
            }
        }

        buf = db_new_with_data(bytes, len);
        free(bytes);
    } else {
        runtime_error("buffer() requires a string or array argument, not %s", value_type_name(data.type));
    }

    return make_buffer(buf);
}

// buffer_from_hex(hex_string) - Create buffer from hex string
value_t builtin_buffer_from_hex(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_from_hex() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t hex_val = args[0];
    if (hex_val.type != VAL_STRING) {
        runtime_error("buffer_from_hex() requires a string argument, not %s", value_type_name(hex_val.type));
    }

    const char* hex_str = hex_val.as.string;
    if (!hex_str) {
        runtime_error("buffer_from_hex() requires a non-null string");
    }

    size_t len = strlen(hex_str);
    db_buffer buf = db_from_hex(hex_str, len);

    if (!buf) {
        runtime_error("Invalid hex string");
    }

    return make_buffer(buf);
}

// buffer_slice(buffer, offset, length) - Create buffer slice
value_t builtin_buffer_slice(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) {
        runtime_error("buffer_slice() takes exactly 3 arguments (%d given)", arg_count);
    }

    value_t buf_val = args[0];
    value_t offset_val = args[1];
    value_t length_val = args[2];

    if (buf_val.type != VAL_BUFFER) {
        runtime_error("buffer_slice() requires a buffer as first argument, not %s", value_type_name(buf_val.type));
    }
    if (offset_val.type != VAL_INT32) {
        runtime_error("buffer_slice() requires an integer offset, not %s", value_type_name(offset_val.type));
    }
    if (length_val.type != VAL_INT32) {
        runtime_error("buffer_slice() requires an integer length, not %s", value_type_name(length_val.type));
    }

    db_buffer buf = buf_val.as.buffer;
    int32_t offset = offset_val.as.int32;
    int32_t length = length_val.as.int32;

    if (offset < 0 || length < 0) {
        runtime_error("buffer_slice() offset and length must be non-negative");
    }

    db_buffer slice = db_slice(buf, (size_t)offset, (size_t)length);
    if (!slice) {
        runtime_error("Invalid buffer slice bounds");
    }

    return make_buffer(slice);
}

// buffer_concat(buf1, buf2) - Concatenate two buffers
value_t builtin_buffer_concat(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("buffer_concat() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t buf1_val = args[0];
    value_t buf2_val = args[1];

    if (buf1_val.type != VAL_BUFFER) {
        runtime_error("buffer_concat() requires buffer as first argument, not %s", value_type_name(buf1_val.type));
    }
    if (buf2_val.type != VAL_BUFFER) {
        runtime_error("buffer_concat() requires buffer as second argument, not %s", value_type_name(buf2_val.type));
    }

    db_buffer result = db_concat(buf1_val.as.buffer, buf2_val.as.buffer);
    return make_buffer(result);
}

// buffer_to_hex(buffer) - Convert buffer to hex string
value_t builtin_buffer_to_hex(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_to_hex() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buf_val = args[0];
    if (buf_val.type != VAL_BUFFER) {
        runtime_error("buffer_to_hex() requires a buffer argument, not %s", value_type_name(buf_val.type));
    }

    db_buffer hex_buf = db_to_hex(buf_val.as.buffer, false); // lowercase

    // Create null-terminated string from hex buffer
    size_t hex_len = db_size(hex_buf);
    char* null_term_hex = malloc(hex_len + 1);
    if (!null_term_hex) {
        db_release(&hex_buf);
        runtime_error("Failed to allocate memory for hex string");
    }

    memcpy(null_term_hex, hex_buf, hex_len);
    null_term_hex[hex_len] = '\0';

    ds_string hex_str = ds_new(null_term_hex);

    free(null_term_hex);
    db_release(&hex_buf);

    return make_string_ds(hex_str);
}

// =============================
// BUFFER BUILDER BUILTIN FUNCTIONS
// =============================

// buffer_builder(capacity) - Create buffer builder
value_t builtin_buffer_builder(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_builder() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t capacity_val = args[0];
    if (capacity_val.type != VAL_INT32) {
        runtime_error("buffer_builder() requires an integer capacity, not %s", value_type_name(capacity_val.type));
    }

    int32_t capacity = capacity_val.as.int32;
    if (capacity < 0) {
        runtime_error("buffer_builder() capacity must be non-negative");
    }

    // Create reference counted builder directly
    db_builder builder = db_builder_new((size_t)capacity);
    return make_buffer_builder(builder);
}

// builder_append_uint8(builder, value) - Append uint8 to builder
value_t builtin_builder_append_uint8(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint8() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint8() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint8() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 255) {
        runtime_error("builder_append_uint8() value must be 0-255, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint8(builder, (uint8_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint16_le(builder, value) - Append uint16 in little-endian
value_t builtin_builder_append_uint16_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint16_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint16_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint16_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 65535) {
        runtime_error("builder_append_uint16_le() value must be 0-65535, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint16_le(builder, (uint16_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint32_le(builder, value) - Append uint32 in little-endian
value_t builtin_builder_append_uint32_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint32_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint32_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint32_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0) {
        runtime_error("builder_append_uint32_le() value must be non-negative, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint32_le(builder, (uint32_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_cstring(builder, string) - Append string to builder
value_t builtin_builder_append_cstring(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_cstring() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t string_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_cstring() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (string_val.type != VAL_STRING) {
        runtime_error("builder_append_cstring() requires a string value, not %s", value_type_name(string_val.type));
    }

    const char* str = string_val.as.string;
    if (!str) {
        runtime_error("builder_append_cstring() requires a non-null string");
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_cstring(builder, str) != 0) {
        runtime_error("Failed to append string to buffer builder");
    }

    return make_null();
}

// builder_finish(builder) - Finish building and get buffer
value_t builtin_builder_finish(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("builder_finish() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_finish() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }

    db_builder builder = builder_val.as.builder;
    db_buffer result = db_builder_finish(&builder);

    // After finish, the builder is invalidated - reference counting handles cleanup

    return make_buffer(result);
}

// ============================
// BUFFER READER BUILTIN FUNCTIONS
// ============================

// buffer_reader(buffer) - Create buffer reader
value_t builtin_buffer_reader(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_reader() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    if (buffer_val.type != VAL_BUFFER) {
        runtime_error("buffer_reader() requires a buffer argument, not %s", value_type_name(buffer_val.type));
    }

    db_reader reader = db_reader_new(buffer_val.as.buffer);
    return make_buffer_reader(reader);
}

// reader_read_uint8(reader) - Read uint8 from reader
value_t builtin_reader_read_uint8(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint8() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint8() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 1)) {
        runtime_error("Cannot read uint8: not enough data remaining");
    }

    uint8_t value = db_read_uint8(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint16_le(reader) - Read uint16 in little-endian from reader
value_t builtin_reader_read_uint16_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint16_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint16_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 2)) {
        runtime_error("Cannot read uint16: not enough data remaining");
    }

    uint16_t value = db_read_uint16_le(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint32_le(reader) - Read uint32 in little-endian from reader
value_t builtin_reader_read_uint32_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint32_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint32_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 4)) {
        runtime_error("Cannot read uint32: not enough data remaining");
    }

    uint32_t value = db_read_uint32_le(reader);

    // Check if value fits in int32_t range
    if (value <= INT32_MAX) {
        return make_int32((int32_t)value);
    } else {
        // Convert to bigint for large values
        di_int big = di_from_uint32(value);
        return make_bigint(big);
    }
}

// reader_position(reader) - Get reader position
value_t builtin_reader_position(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_position() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_position() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t pos = db_reader_position(reader_val.as.reader);
    return make_int32((int32_t)pos);
}

// reader_remaining(reader) - Get remaining bytes in reader
value_t builtin_reader_remaining(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_remaining() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_remaining() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t remaining = db_reader_remaining(reader_val.as.reader);
    return make_int32((int32_t)remaining);
}

// ===================
// I/O BUILTIN FUNCTIONS
// ===================

// read_file(filename) - Read file into buffer
value_t builtin_read_file(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("read_file() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t filename_val = args[0];
    if (filename_val.type != VAL_STRING) {
        runtime_error("read_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error("read_file() requires a non-null filename");
    }

    db_buffer buf = db_read_file(filename);
    if (!buf) {
        runtime_error("Failed to read file: %s", filename);
    }

    return make_buffer(buf);
}

// write_file(buffer, filename) - Write buffer to file
value_t builtin_write_file(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("write_file() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    value_t filename_val = args[1];

    if (buffer_val.type != VAL_BUFFER) {
        runtime_error("write_file() requires a buffer as first argument, not %s", value_type_name(buffer_val.type));
    }
    if (filename_val.type != VAL_STRING) {
        runtime_error("write_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error("write_file() requires a non-null filename");
    }

    bool success = db_write_file(buffer_val.as.buffer, filename);
    return make_boolean(success);
}
