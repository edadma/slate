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

// Call a built-in function by name
value_t call_builtin(bitty_vm* vm, const char* name, int arg_count, value_t* args) {
    // For now, dispatch manually to the functions
    // In a full implementation, this would look up the function from globals
    
    if (strcmp(name, "print") == 0) {
        return builtin_print(vm, arg_count, args);
    } else if (strcmp(name, "type") == 0) {
        return builtin_type(vm, arg_count, args);
    } else if (strcmp(name, "abs") == 0) {
        return builtin_abs(vm, arg_count, args);
    } else if (strcmp(name, "sqrt") == 0) {
        return builtin_sqrt(vm, arg_count, args);
    } else if (strcmp(name, "floor") == 0) {
        return builtin_floor(vm, arg_count, args);
    } else if (strcmp(name, "ceil") == 0) {
        return builtin_ceil(vm, arg_count, args);
    } else if (strcmp(name, "round") == 0) {
        return builtin_round(vm, arg_count, args);
    } else if (strcmp(name, "min") == 0) {
        return builtin_min(vm, arg_count, args);
    } else if (strcmp(name, "max") == 0) {
        return builtin_max(vm, arg_count, args);
    } else if (strcmp(name, "random") == 0) {
        return builtin_random(vm, arg_count, args);
    }
    
    // Unknown function
    printf("Runtime error: Unknown function '%s'\n", name);
    return make_null();
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
    if (arg.type != VAL_NUMBER) {
        printf("Runtime error: abs() requires a number argument\n");
        return make_null();
    }
    
    return make_number(fabs(arg.as.number));
}

// sqrt(number) - Square root
value_t builtin_sqrt(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: sqrt() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_NUMBER) {
        printf("Runtime error: sqrt() requires a number argument\n");
        return make_null();
    }
    
    if (arg.as.number < 0) {
        printf("Runtime error: sqrt() of negative number\n");
        return make_null();
    }
    
    return make_number(sqrt(arg.as.number));
}

// floor(number) - Floor function
value_t builtin_floor(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: floor() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_NUMBER) {
        printf("Runtime error: floor() requires a number argument\n");
        return make_null();
    }
    
    return make_number(floor(arg.as.number));
}

// ceil(number) - Ceiling function
value_t builtin_ceil(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: ceil() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_NUMBER) {
        printf("Runtime error: ceil() requires a number argument\n");
        return make_null();
    }
    
    return make_number(ceil(arg.as.number));
}

// round(number) - Round to nearest integer
value_t builtin_round(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        printf("Runtime error: round() takes exactly 1 argument (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t arg = args[0];
    if (arg.type != VAL_NUMBER) {
        printf("Runtime error: round() requires a number argument\n");
        return make_null();
    }
    
    return make_number(round(arg.as.number));
}

// min(a, b) - Minimum of two values
value_t builtin_min(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        printf("Runtime error: min() takes exactly 2 arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        printf("Runtime error: min() requires number arguments\n");
        return make_null();
    }
    
    return make_number(a.as.number < b.as.number ? a.as.number : b.as.number);
}

// max(a, b) - Maximum of two values
value_t builtin_max(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        printf("Runtime error: max() takes exactly 2 arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    value_t a = args[0];
    value_t b = args[1];
    
    if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        printf("Runtime error: max() requires number arguments\n");
        return make_null();
    }
    
    return make_number(a.as.number > b.as.number ? a.as.number : b.as.number);
}

// random() - Random number 0.0 to 1.0
value_t builtin_random(bitty_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        printf("Runtime error: random() takes no arguments (%d given)\n", arg_count);
        return make_null();
    }
    
    return make_number((double)rand() / RAND_MAX);
}