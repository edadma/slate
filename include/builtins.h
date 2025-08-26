#ifndef BITTY_BUILTINS_H
#define BITTY_BUILTINS_H

#include "vm.h"

// Built-in function signature
typedef value_t (*builtin_func_t)(bitty_vm* vm, int arg_count, value_t* args);

// Built-in function entry
typedef struct {
    const char* name;
    builtin_func_t func;
    int min_args;
    int max_args; // -1 for unlimited
} builtin_entry_t;

// Initialize built-in functions in VM
void builtins_init(bitty_vm* vm);

// Built-in function implementations
value_t builtin_print(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_type(bitty_vm* vm, int arg_count, value_t* args);

// Math functions
value_t builtin_abs(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_sqrt(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_floor(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_ceil(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_round(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_min(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_max(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_random(bitty_vm* vm, int arg_count, value_t* args);

// Trigonometric functions
value_t builtin_sin(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_cos(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_tan(bitty_vm* vm, int arg_count, value_t* args);

// Input/conversion functions
value_t builtin_input(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_parse_int(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_parse_number(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_args(bitty_vm* vm, int arg_count, value_t* args);

// Iterator functions
value_t builtin_iterator(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_has_next(bitty_vm* vm, int arg_count, value_t* args);
value_t builtin_next(bitty_vm* vm, int arg_count, value_t* args);

// Runtime error handling
void runtime_error(const char* message, ...);

// Helper functions
void register_builtin(bitty_vm* vm, const char* name, builtin_func_t func, int min_args, int max_args);

#endif // BITTY_BUILTINS_H