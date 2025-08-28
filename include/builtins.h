#ifndef SLATE_BUILTINS_H
#define SLATE_BUILTINS_H

#include "vm.h"

// Built-in function signature
typedef value_t (*native_t)(slate_vm* vm, int arg_count, value_t* args);

// Built-in function entry
typedef struct {
    const char* name;
    native_t func;
    int min_args;
    int max_args; // -1 for unlimited
} builtin_entry_t;

// Initialize built-in functions in VM
void builtins_init(slate_vm* vm);

// Built-in function implementations
value_t builtin_print(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_type(slate_vm* vm, int arg_count, value_t* args);

// Math functions
value_t builtin_abs(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_sqrt(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_floor(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_ceil(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_round(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_min(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_max(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_random(slate_vm* vm, int arg_count, value_t* args);

// Trigonometric functions
value_t builtin_sin(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_cos(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_tan(slate_vm* vm, int arg_count, value_t* args);

// Inverse trigonometric functions
value_t builtin_asin(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_acos(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_atan(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_atan2(slate_vm* vm, int arg_count, value_t* args);

// Angle conversion functions
value_t builtin_degrees(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_radians(slate_vm* vm, int arg_count, value_t* args);

// Exponential and logarithmic functions
value_t builtin_exp(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_ln(slate_vm* vm, int arg_count, value_t* args);

// Sign function
value_t builtin_sign(slate_vm* vm, int arg_count, value_t* args);

// Input/conversion functions
value_t builtin_input(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_parse_int(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_parse_number(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_args(slate_vm* vm, int arg_count, value_t* args);

// Iterator functions
value_t builtin_iterator(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_has_next(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_next(slate_vm* vm, int arg_count, value_t* args);

// Buffer functions
value_t builtin_buffer(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_from_hex(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_slice(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_concat(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_to_hex(slate_vm* vm, int arg_count, value_t* args);

// Buffer builder functions
value_t builtin_buffer_builder(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint8(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint16_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint32_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_cstring(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_finish(slate_vm* vm, int arg_count, value_t* args);

// Buffer reader functions
value_t builtin_buffer_reader(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint8(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint16_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint32_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_position(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_remaining(slate_vm* vm, int arg_count, value_t* args);

// I/O functions
value_t builtin_read_file(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_write_file(slate_vm* vm, int arg_count, value_t* args);

// Runtime error handling
void runtime_error(const char* message, ...);

// Helper functions
void register_builtin(slate_vm* vm, const char* name, native_t func, int min_args, int max_args);

#endif // SLATE_BUILTINS_H