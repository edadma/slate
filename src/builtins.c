#include "builtins.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Static random initialization flag
static int random_initialized = 0;

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
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: print() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t arg = args[0];
    
    switch (arg.type) {
    case VAL_INT32:
        printf("%d\n", arg.as.int32);
        break;
    case VAL_BIGINT: {
        // Try to convert BigInt to int64 for display
        int64_t int_val;
        if (db_to_int64(arg.as.bigint, &int_val)) {
            printf("%lld\n", (long long)int_val);
        } else {
            // Fall back to double display (lossy)
            double double_val = db_to_double(arg.as.bigint);
            printf("%.0f\n", double_val);
        }
        break;
    }
    case VAL_NUMBER:
        if (arg.as.number == (int)arg.as.number) {
            printf("%d\n", (int)arg.as.number);
        } else {
            printf("%.6g\n", arg.as.number);
        }
        break;
    case VAL_STRING:
        printf("%s\n", arg.as.string);
        break;
    case VAL_BOOLEAN:
        printf("%s\n", arg.as.boolean ? "true" : "false");
        break;
    case VAL_NULL:
        printf("null\n");
        break;
    case VAL_UNDEFINED:
        printf("undefined\n");
        break;
    case VAL_ARRAY:
        printf("[Array with %d elements]\n", da_length(arg.as.array));
        break;
    case VAL_OBJECT:
        printf("[Object]\n");
        break;
    case VAL_FUNCTION:
        printf("[Function]\n");
        break;
    case VAL_CLOSURE:
        printf("[Closure]\n");
        break;
    default:
        printf("[Unknown type]\n");
        break;
    }
    
    return make_null(); // print returns null/void
}

// type(value) - Get type name as string
value_t builtin_type(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: type() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
    default:
        type_name = "unknown";
        break;
    }
    
    return make_string(type_name);
}

// abs(number) - Absolute value
value_t builtin_abs(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: abs() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: abs() requires a number argument\n");
        return make_null();
    }
}

// sqrt(number) - Square root
value_t builtin_sqrt(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: sqrt() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: sqrt() requires a number argument\n");
        return make_null();
    }
    
    if (val < 0) {
        printf("Runtime error: sqrt() of negative number\n");
        return make_null();
    }
    
    return make_number(sqrt(val));
}

// floor(number) - Floor function
value_t builtin_floor(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: floor() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: floor() requires a number argument\n");
        return make_null();
    }
}

// ceil(number) - Ceiling function
value_t builtin_ceil(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: ceil() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: ceil() requires a number argument\n");
        return make_null();
    }
}

// round(number) - Round to nearest integer
value_t builtin_round(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: round() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: round() requires a number argument\n");
        return make_null();
    }
}

// min(a, b) - Minimum of two values
value_t builtin_min(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        printf("Runtime error: min() takes exactly 2 arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    // Check if both are numeric types
    if (!((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
          (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER))) {
        printf("Runtime error: min() requires number arguments\n");
        return make_null();
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
        printf("Runtime error: max() takes exactly 2 arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    // Check if both are numeric types
    if (!((a.type == VAL_INT32 || a.type == VAL_BIGINT || a.type == VAL_NUMBER) &&
          (b.type == VAL_INT32 || b.type == VAL_BIGINT || b.type == VAL_NUMBER))) {
        printf("Runtime error: max() requires number arguments\n");
        return make_null();
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
        printf("Runtime error: random() takes no arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    return make_number((double)rand() / RAND_MAX);
}

// sin(number) - Sine function (radians)
value_t builtin_sin(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: sin() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: sin() requires a number argument\n");
        return make_null();
    }
    
    return make_number(sin(val));
}

// cos(number) - Cosine function (radians)
value_t builtin_cos(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: cos() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: cos() requires a number argument\n");
        return make_null();
    }
    
    return make_number(cos(val));
}

// tan(number) - Tangent function (radians)
value_t builtin_tan(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: tan() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
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
        printf("Runtime error: tan() requires a number argument\n");
        return make_null();
    }
    
    return make_number(tan(val));
}