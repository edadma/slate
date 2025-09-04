#include "builtins.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "array.h"
#include "buffer.h"
#include "buffer_builder.h"
#include "buffer_reader.h"
#include "classes/String/class_string.h"
#include "classes/StringBuilder/string_builder.h"
#include "classes/Boolean/class_boolean.h"
#include "datetime.h"
#include "int.h"
#include "iterator.h"
#include "classes/Number/number.h"
#include "classes/Float/float.h"
#include "library_assert.h"
#include "local_date.h"
#include "local_datetime.h"
#include "local_time.h"
#include "instant.h"
#include "range.h"
#include "runtime_error.h"
#include "value.h"
#include "classes/Value/value.h"
#include "classes/Null/class_null.h"
#include "classes/Object/class_object.h"

// Include dynamic_string for ds_builder functions (implementation in library_impl.c)
#include "dynamic_string.h"

// Include datetime for date/time functions
#include "datetime.h"

// Static random initialization flag
static int random_initialized = 0;


// Register a built-in function in the VM's global namespace
void register_builtin(vm_t* vm, const char* name, native_t func, int min_args, int max_args) {
    // Create a built-in function value
    value_t builtin_val = make_native((void*)func);

    // Store in the VM's global namespace
    do_set(vm->globals, name, &builtin_val, sizeof(value_t));
}

// Global String class storage
value_t* global_string_class = NULL;

// Global Boolean class storage
value_t* global_boolean_class = NULL;

// Global Value class storage
value_t* global_value_class = NULL;

// Initialize all built-in functions
void builtins_init(vm_t* vm) {
    // Initialize random seed once
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }

    // Create the String class with its prototype
    do_object string_proto = do_create(NULL);

    // Add methods to String prototype
    value_t hash_method = make_native(builtin_string_hash);
    do_set(string_proto, "hash", &hash_method, sizeof(value_t));
    
    value_t equals_method = make_native(builtin_string_equals);
    do_set(string_proto, "equals", &equals_method, sizeof(value_t));
    
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
    value_t string_class = make_class("String", string_proto, NULL);

    // Set the factory function to allow String(codepoint) or String([codepoints])
    string_class.as.class->factory = string_factory;

    // Store in globals
    do_set(vm->globals, "String", &string_class, sizeof(value_t));

    // Store a global reference for use in make_string
    static value_t string_class_storage;
    string_class_storage = vm_retain(string_class);
    global_string_class = &string_class_storage;

    // Create the Boolean class with its prototype
    do_object boolean_proto = do_create(NULL);

    // Add methods to Boolean prototype
    value_t boolean_hash_method = make_native(builtin_boolean_hash);
    do_set(boolean_proto, "hash", &boolean_hash_method, sizeof(value_t));
    
    value_t boolean_equals_method = make_native(builtin_boolean_equals);
    do_set(boolean_proto, "equals", &boolean_equals_method, sizeof(value_t));
    
    value_t boolean_to_string_method = make_native(builtin_boolean_to_string);
    do_set(boolean_proto, "toString", &boolean_to_string_method, sizeof(value_t));

    value_t boolean_and_method = make_native(builtin_boolean_and);
    do_set(boolean_proto, "and", &boolean_and_method, sizeof(value_t));

    value_t boolean_or_method = make_native(builtin_boolean_or);
    do_set(boolean_proto, "or", &boolean_or_method, sizeof(value_t));

    value_t boolean_not_method = make_native(builtin_boolean_not);
    do_set(boolean_proto, "not", &boolean_not_method, sizeof(value_t));

    value_t boolean_xor_method = make_native(builtin_boolean_xor);
    do_set(boolean_proto, "xor", &boolean_xor_method, sizeof(value_t));

    // Create the Boolean class
    value_t boolean_class = make_class("Boolean", boolean_proto, NULL);

    // Set the factory function to allow Boolean() or Boolean(value)
    boolean_class.as.class->factory = boolean_factory;

    // Store in globals
    do_set(vm->globals, "Boolean", &boolean_class, sizeof(value_t));

    // Store a global reference for use in make_boolean
    static value_t boolean_class_storage;
    boolean_class_storage = vm_retain(boolean_class);
    global_boolean_class = &boolean_class_storage;

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

    // Initialize LocalDateTime class
    local_datetime_class_init(vm);
    // Initialize Instant class
    init_instant_class(vm);

    // Initialize Zone and Date classes
    init_datetime_classes(vm);

    // Initialize Buffer class
    buffer_class_init(vm);

    // Initialize BufferReader class
    buffer_reader_class_init(vm);

    // Initialize Null class
    initialize_null_class(vm);
    
    // Initialize Object class
    initialize_object_class(vm);

    // Register all built-ins
    register_builtin(vm, "print", builtin_print, 1, 1);
    register_builtin(vm, "type", builtin_type, 1, 1);
    
    // Math functions in global namespace
    register_builtin(vm, "abs", builtin_abs, 1, 1);
    register_builtin(vm, "sqrt", builtin_sqrt, 1, 1);
    register_builtin(vm, "floor", builtin_floor, 1, 1);
    register_builtin(vm, "ceil", builtin_ceil, 1, 1);
    register_builtin(vm, "round", builtin_round, 1, 1);
    register_builtin(vm, "sin", builtin_sin, 1, 1);
    register_builtin(vm, "cos", builtin_cos, 1, 1);
    register_builtin(vm, "tan", builtin_tan, 1, 1);
    register_builtin(vm, "asin", builtin_asin, 1, 1);
    register_builtin(vm, "acos", builtin_acos, 1, 1);
    register_builtin(vm, "atan", builtin_atan, 1, 1);
    register_builtin(vm, "atan2", builtin_atan2, 2, 2);
    register_builtin(vm, "exp", builtin_exp, 1, 1);
    register_builtin(vm, "ln", builtin_ln, 1, 1);
    register_builtin(vm, "degrees", builtin_degrees, 1, 1);
    register_builtin(vm, "radians", builtin_radians, 1, 1);
    register_builtin(vm, "sign", builtin_sign, 1, 1);
    
    // min/max are both global functions and methods (for convenience)
    register_builtin(vm, "min", builtin_min, 2, 2);
    register_builtin(vm, "max", builtin_max, 2, 2);
    register_builtin(vm, "random", builtin_random, 0, 0);
    
    register_builtin(vm, "input", builtin_input, 0, 1);
    register_builtin(vm, "parse_int", builtin_parse_int, 1, 1);
    register_builtin(vm, "parse_number", builtin_parse_number, 1, 1);
    register_builtin(vm, "args", builtin_args, 0, 0);


    // I/O functions
    register_builtin(vm, "read_file", builtin_read_file, 1, 1);
    register_builtin(vm, "write_file", builtin_write_file, 2, 2);

    // Create the Value class - the ultimate superclass of all values
    do_object value_proto = do_create(NULL);

    // Add methods to Value prototype
    value_t value_to_string_method = make_native(builtin_value_to_string);
    do_set(value_proto, "toString", &value_to_string_method, sizeof(value_t));
    
    value_t value_equals_method = make_native(builtin_value_equals);
    do_set(value_proto, "equals", &value_equals_method, sizeof(value_t));

    // Value prototype has toString and equals - numeric methods inherited from Number

    // Create the Value class
    value_t value_class = make_class("Value", value_proto, NULL);

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

    // Initialize Number class (abstract superclass)
    number_class_init(vm);
    
    // Initialize Int class (inherits from Number)
    int_class_init(vm);
    
    // Initialize Float class (inherits from Number)
    float_class_init(vm);

    // Now that Value class is created, set up proper inheritance chain
    // Array class should inherit from Value class
    if (global_array_class && global_array_class->type == VAL_CLASS) {
        global_array_class->class = &value_class_storage;
    }
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "print() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Use the specialized print function for builtins
    print_for_builtin(vm, arg);
    printf("\n");

    return make_undefined(); // print returns void
}


// abs(number) - Absolute value
value_t builtin_abs(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "abs() takes exactly 1 argument (%d given)", arg_count);
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
    } else if (arg.type == VAL_FLOAT64) {
        return make_float64(fabs(arg.as.float64));
    } else {
        runtime_error(vm, "abs() requires a number argument");
    }
}

// sqrt(number) - Square root
value_t builtin_sqrt(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sqrt() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "sqrt() requires a number argument");
    }

    double val = value_to_float64(arg);

    if (val < 0) {
        runtime_error(vm, "sqrt() of negative number");
    }

    return make_float64(sqrt(val));
}

// floor(number) - Floor function
value_t builtin_floor(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "floor() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "floored"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "floored"
        return arg;
    } else if (arg.type == VAL_FLOAT64) {
        double result = floor(arg.as.float64);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_float64(result);
        }
    } else {
        runtime_error(vm, "floor() requires a number argument");
    }
}

// ceil(number) - Ceiling function
value_t builtin_ceil(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "ceil() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "ceiled"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "ceiled"
        return arg;
    } else if (arg.type == VAL_FLOAT64) {
        double result = ceil(arg.as.float64);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_float64(result);
        }
    } else {
        runtime_error(vm, "ceil() requires a number argument");
    }
}

// round(number) - Round to nearest integer
value_t builtin_round(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "round() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "rounded"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "rounded"
        return arg;
    } else if (arg.type == VAL_FLOAT64) {
        double result = round(arg.as.float64);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_float64(result);
        }
    } else {
        runtime_error(vm, "round() requires a number argument");
    }
}

// min(a, b) - Minimum of two values
value_t builtin_min(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "min() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error(vm, "min() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_float64(a);
    double b_val = value_to_float64(b);

    // Return the smaller value with its original type
    return (a_val < b_val) ? a : b;
}

// max(a, b) - Maximum of two values
value_t builtin_max(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "max() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error(vm, "max() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_float64(a);
    double b_val = value_to_float64(b);

    // Return the larger value with its original type
    return (a_val > b_val) ? a : b;
}

// random() - Random number 0.0 to 1.0
value_t builtin_random(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "random() takes no arguments (%d given)", arg_count);
    }

    return make_float64((double)rand() / RAND_MAX);
}

// sin(number) - Sine function (radians)
value_t builtin_sin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "sin() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(sin(val));
}

// cos(number) - Cosine function (radians)
value_t builtin_cos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "cos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "cos() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(cos(val));
}

// tan(number) - Tangent function (radians)
value_t builtin_tan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "tan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "tan() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(tan(val));
}

// exp(number) - Exponential function (e^x)
value_t builtin_exp(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "exp() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "exp() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(exp(val));
}

// ln(number) - Natural logarithm (base e)
value_t builtin_ln(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "ln() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "ln() requires a number argument");
    }

    double val = value_to_float64(arg);

    // Check for domain error (ln of non-positive numbers)
    if (val <= 0) {
        runtime_error(vm, "ln() domain error: argument must be positive");
    }

    return make_float64(log(val));
}

// asin(number) - Inverse sine function (returns radians)
value_t builtin_asin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "asin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "asin() requires a number argument");
    }

    double val = value_to_float64(arg);

    // Check for domain error (asin domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error(vm, "asin() domain error: argument must be in range [-1, 1]");
    }

    return make_float64(asin(val));
}

// acos(number) - Inverse cosine function (returns radians)
value_t builtin_acos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "acos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "acos() requires a number argument");
    }

    double val = value_to_float64(arg);

    // Check for domain error (acos domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error(vm, "acos() domain error: argument must be in range [-1, 1]");
    }

    return make_float64(acos(val));
}

// atan(number) - Inverse tangent function (returns radians)
value_t builtin_atan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "atan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "atan() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(atan(val));
}

// atan2(y, x) - Two-argument inverse tangent function (returns radians)
value_t builtin_atan2(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "atan2() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t y = args[0];
    value_t x = args[1];

    // Check if both are numeric types
    if (!is_number(y) || !is_number(x)) {
        runtime_error(vm, "atan2() requires number arguments");
    }

    double y_val = value_to_float64(y);
    double x_val = value_to_float64(x);

    return make_float64(atan2(y_val, x_val));
}

// degrees(radians) - Convert radians to degrees
value_t builtin_degrees(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "degrees() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "degrees() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(val * 180.0 / M_PI);
}

// radians(degrees) - Convert degrees to radians
value_t builtin_radians(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "radians() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "radians() requires a number argument");
    }

    double val = value_to_float64(arg);

    return make_float64(val * M_PI / 180.0);
}

// sign(number) - Sign function (-1, 0, or 1)
value_t builtin_sign(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sign() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error(vm, "sign() requires a number argument");
    }

    double val = value_to_float64(arg);

    if (val > 0.0) {
        return make_int32(1);
    } else if (val < 0.0) {
        return make_int32(-1);
    } else {
        return make_int32(0);
    }
}

// input(prompt) - Read user input with optional prompt
value_t builtin_input(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count > 1) {
        runtime_error(vm, "input() takes 0 or 1 arguments (%d given)", arg_count);
    }

    // Print prompt if provided
    if (arg_count == 1) {
        value_t prompt = args[0];
        if (prompt.type == VAL_STRING) {
            printf("%s", prompt.as.string);
            fflush(stdout);
        } else {
            runtime_error(vm, "input() prompt must be a string");
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
value_t builtin_parse_int(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "parse_int() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error(vm, "parse_int() requires a string argument");
    }

    char* endptr;
    const char* str = arg.as.string;

    // Try parsing as long long first
    long long val = strtoll(str, &endptr, 10);

    // Check if entire string was consumed
    if (*endptr != '\0') {
        runtime_error(vm, "'%s' is not a valid integer", str);
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
value_t builtin_parse_number(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "parse_number() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error(vm, "parse_number() requires a string argument");
    }

    const char* str = arg.as.string;
    char* endptr;

    // Check if it contains a decimal point
    if (strchr(str, '.') != NULL) {
        // Parse as float
        double val = strtod(str, &endptr);

        if (*endptr != '\0') {
            runtime_error(vm, "'%s' is not a valid number", str);
        }

        return make_float64(val);
    } else {
        // Parse as integer first, then convert to appropriate type
        long long val = strtoll(str, &endptr, 10);

        if (*endptr != '\0') {
            runtime_error(vm, "'%s' is not a valid number", str);
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
value_t builtin_args(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "args() takes no arguments (%d given)", arg_count);
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
value_t builtin_read_file(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "read_file() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t filename_val = args[0];
    if (filename_val.type != VAL_STRING) {
        runtime_error(vm, "read_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error(vm, "read_file() requires a non-null filename");
    }

    db_buffer buf = db_read_file(filename);
    if (!buf) {
        runtime_error(vm, "Failed to read file: %s", filename);
    }

    return make_buffer(buf);
}

// write_file(buffer, filename) - Write buffer to file
value_t builtin_write_file(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "write_file() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    value_t filename_val = args[1];

    if (buffer_val.type != VAL_BUFFER) {
        runtime_error(vm, "write_file() requires a buffer as first argument, not %s", value_type_name(buffer_val.type));
    }
    if (filename_val.type != VAL_STRING) {
        runtime_error(vm, "write_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error(vm, "write_file() requires a non-null filename");
    }

    bool success = db_write_file(buffer_val.as.buffer, filename);
    return make_boolean(success);
}

// Array method: length()
// Returns the length of the array as an int32


// String method: isEmpty()
// Returns true if string has no characters
