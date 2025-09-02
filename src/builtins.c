#include "builtins.h"
#include "datetime.h"
#include "classes/String/class_string.h"
#include "array.h"
#include "buffer.h"
#include "buffer_builder.h"
#include "buffer_reader.h"
#include "range.h"
#include "iterator.h"
#include "value.h"
#include "int.h"
#include "local_date.h"
#include "local_time.h"
#include <assert.h>
#include "runtime_error.h"
#include "library_assert.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include dynamic_string for ds_builder functions (implementation in library_impl.c)
#include "dynamic_string.h"

// Include datetime for date/time functions
#include "datetime.h"

// Static random initialization flag
static int random_initialized = 0;



// Runtime error handling - uses context-aware error system
void runtime_error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    
    char formatted_message[256];
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    if (g_current_vm) {
        // Use the new context-aware error system
        slate_runtime_error(g_current_vm, ERR_TYPE, __FILE__, __LINE__, -1, "%s", formatted_message);
    } else {
        // Fallback for cases without VM
        fprintf(stderr, "Runtime error: %s\n", formatted_message);
        exit(1);
    }
}

// Register a built-in function in the VM's global namespace
void register_builtin(slate_vm* vm, const char* name, native_t func, int min_args, int max_args) {
    // Create a built-in function value
    value_t builtin_val = make_native((void*)func);

    // Store in the VM's global namespace
    do_set(vm->globals, name, &builtin_val, sizeof(value_t));
}

// Global String class storage
value_t* global_string_class = NULL;


// Global Range class storage  



















// Helper function to compute GCD using Euclidean algorithm
static int32_t compute_gcd(int32_t a, int32_t b) {
    a = a < 0 ? -a : a; // Make positive
    b = b < 0 ? -b : b; // Make positive
    
    while (b != 0) {
        int32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}














// Global Value class storage  
value_t* global_value_class = NULL;

// Initialize all built-in functions
void builtins_init(slate_vm* vm) {
    // Initialize random seed once
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }

    // Create the String class with its prototype
    do_object string_proto = do_create(NULL);

    // Add methods to String prototype
    value_t length_method = make_native(builtin_string_length);
    do_set(string_proto, "length", &length_method, sizeof(value_t));

    value_t substring_method = make_native(builtin_string_substring);
    do_set(string_proto, "substring", &substring_method, sizeof(value_t));

    value_t to_upper_method = make_native(builtin_string_to_upper);
    do_set(string_proto, "toUpper", &to_upper_method, sizeof(value_t));

    value_t to_lower_method = make_native(builtin_string_to_lower);
    do_set(string_proto, "toLower", &to_lower_method, sizeof(value_t));

    value_t trim_method = make_native(builtin_string_trim);
    do_set(string_proto, "trim", &trim_method, sizeof(value_t));

    value_t starts_with_method = make_native(builtin_string_starts_with);
    do_set(string_proto, "startsWith", &starts_with_method, sizeof(value_t));

    value_t ends_with_method = make_native(builtin_string_ends_with);
    do_set(string_proto, "endsWith", &ends_with_method, sizeof(value_t));

    value_t contains_method = make_native(builtin_string_contains);
    do_set(string_proto, "contains", &contains_method, sizeof(value_t));

    value_t replace_method = make_native(builtin_string_replace);
    do_set(string_proto, "replace", &replace_method, sizeof(value_t));

    value_t index_of_method = make_native(builtin_string_index_of);
    do_set(string_proto, "indexOf", &index_of_method, sizeof(value_t));

    value_t string_is_empty_method = make_native(builtin_string_is_empty);
    do_set(string_proto, "isEmpty", &string_is_empty_method, sizeof(value_t));

    value_t string_non_empty_method = make_native(builtin_string_non_empty);
    do_set(string_proto, "nonEmpty", &string_non_empty_method, sizeof(value_t));

    // Create the String class
    value_t string_class = make_class("String", string_proto);
    
    // Set the factory function to allow String(codepoint) or String([codepoints])
    string_class.as.class->factory = string_factory;

    // Store in globals
    do_set(vm->globals, "String", &string_class, sizeof(value_t));

    // Store a global reference for use in make_string
    static value_t string_class_storage;
    string_class_storage = vm_retain(string_class);
    global_string_class = &string_class_storage;

    // Initialize Array class
    array_class_init(vm);

    // Initialize Range class
    range_class_init(vm);

    // Initialize Iterator class
    iterator_class_init(vm);

    // Initialize StringBuilder class
    string_builder_class_init(vm);

    // Initialize LocalDate class
    local_date_class_init(vm);

    // Initialize LocalTime class
    local_time_class_init(vm);

    // Initialize Buffer class
    buffer_class_init(vm);

    // Initialize BufferReader class
    buffer_reader_class_init(vm);

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


    // I/O functions
    register_builtin(vm, "read_file", builtin_read_file, 1, 1);
    register_builtin(vm, "write_file", builtin_write_file, 2, 2);
    
    // Initialize Int class
    int_class_init(vm);
    

    // Create the Value class - the ultimate superclass of all values
    do_object value_proto = do_create(NULL);
    
    // Add methods to Value prototype
    value_t value_to_string_method = make_native(builtin_value_to_string);
    do_set(value_proto, "toString", &value_to_string_method, sizeof(value_t));
    
    // Create the Value class
    value_t value_class = make_class("Value", value_proto);
    
    // Value class has no factory (factory = NULL) - cannot be instantiated directly
    value_class.as.class->factory = NULL;
    
    // Value class should have no parent class (it's the root class)
    value_class.class = NULL;
    
    // Store in globals (though it shouldn't be called directly)
    do_set(vm->globals, "Value", &value_class, sizeof(value_t));
    
    // Store a global reference for use in make_value functions
    static value_t value_class_storage;
    value_class_storage = vm_retain(value_class);
    global_value_class = &value_class_storage;
    
    // Now that Value class is created, set up proper inheritance chain
    // Array class should inherit from Value class
    if (global_array_class && global_array_class->type == VAL_CLASS) {
        global_array_class->class = &value_class_storage;
    }
    
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("print() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Use the specialized print function for builtins
    print_for_builtin(vm, arg);
    printf("\n");

    return make_undefined(); // print returns void
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

// String method: length
// This will be used as a method on the String prototype





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

// Array method: length()
// Returns the length of the array as an int32













// String method: isEmpty()
// Returns true if string has no characters












