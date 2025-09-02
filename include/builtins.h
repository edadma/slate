#ifndef SLATE_BUILTINS_H
#define SLATE_BUILTINS_H

#include "vm.h"
#include "runtime_error.h"

// Built-in function entry
typedef struct {
    const char* name;
    native_t func;
    int min_args;
    int max_args; // -1 for unlimited
} builtin_entry_t;

// Initialize built-in functions in VM
void builtins_init(vm_t* vm);

// Built-in function implementations
value_t builtin_print(vm_t* vm, int arg_count, value_t* args);
value_t builtin_type(vm_t* vm, int arg_count, value_t* args);

// Math functions
value_t builtin_abs(vm_t* vm, int arg_count, value_t* args);
value_t builtin_sqrt(vm_t* vm, int arg_count, value_t* args);
value_t builtin_floor(vm_t* vm, int arg_count, value_t* args);
value_t builtin_ceil(vm_t* vm, int arg_count, value_t* args);
value_t builtin_round(vm_t* vm, int arg_count, value_t* args);
value_t builtin_min(vm_t* vm, int arg_count, value_t* args);
value_t builtin_max(vm_t* vm, int arg_count, value_t* args);
value_t builtin_random(vm_t* vm, int arg_count, value_t* args);

// Trigonometric functions
value_t builtin_sin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_cos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_tan(vm_t* vm, int arg_count, value_t* args);

// Inverse trigonometric functions
value_t builtin_asin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_acos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_atan(vm_t* vm, int arg_count, value_t* args);
value_t builtin_atan2(vm_t* vm, int arg_count, value_t* args);

// Angle conversion functions
value_t builtin_degrees(vm_t* vm, int arg_count, value_t* args);
value_t builtin_radians(vm_t* vm, int arg_count, value_t* args);

// Exponential and logarithmic functions
value_t builtin_exp(vm_t* vm, int arg_count, value_t* args);
value_t builtin_ln(vm_t* vm, int arg_count, value_t* args);

// Sign function
value_t builtin_sign(vm_t* vm, int arg_count, value_t* args);

// Input/conversion functions
value_t builtin_input(vm_t* vm, int arg_count, value_t* args);
value_t builtin_parse_int(vm_t* vm, int arg_count, value_t* args);
value_t builtin_parse_number(vm_t* vm, int arg_count, value_t* args);
value_t builtin_args(vm_t* vm, int arg_count, value_t* args);

// String methods (for prototypal inheritance)
value_t builtin_string_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_substring(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_to_upper(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_to_lower(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_trim(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_starts_with(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_ends_with(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_contains(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_replace(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_index_of(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_non_empty(vm_t* vm, int arg_count, value_t* args);

// Array methods (for prototypal inheritance)
value_t builtin_array_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_push(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_pop(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_non_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_index_of(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_contains(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_copy(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_slice(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_reverse(vm_t* vm, int arg_count, value_t* args);

// Iterator functions
value_t builtin_iterator(vm_t* vm, int arg_count, value_t* args);
value_t builtin_has_next(vm_t* vm, int arg_count, value_t* args);
value_t builtin_next(vm_t* vm, int arg_count, value_t* args);

// Range functions
value_t builtin_range_start(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_end(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_is_exclusive(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_contains(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_to_array(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_reverse(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_equals(vm_t* vm, int arg_count, value_t* args);

// Iterator functions
value_t builtin_iterator_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_iterator_to_array(vm_t* vm, int arg_count, value_t* args);

// Buffer functions
value_t builtin_buffer(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_from_hex(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_slice(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_concat(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_to_hex(vm_t* vm, int arg_count, value_t* args);

// Buffer class methods (for prototypal inheritance)
value_t builtin_buffer_method_slice(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_concat(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_hex(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_reader(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_string(vm_t* vm, int arg_count, value_t* args);

// Buffer builder functions
value_t builtin_buffer_builder(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_cstring(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_finish(vm_t* vm, int arg_count, value_t* args);

// Buffer reader functions
value_t builtin_buffer_reader(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_position(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_remaining(vm_t* vm, int arg_count, value_t* args);

// I/O functions
value_t builtin_read_file(vm_t* vm, int arg_count, value_t* args);
value_t builtin_write_file(vm_t* vm, int arg_count, value_t* args);

// Runtime error handling

// Helper functions
void register_builtin(vm_t* vm, const char* name, native_t func, int min_args, int max_args);

// Global class references (for use in make_* functions)
extern value_t* global_value_class;
extern value_t* global_string_class;
extern value_t* global_array_class;
extern value_t* global_string_builder_class;
extern value_t* global_buffer_class;
extern value_t* global_int_class;

#endif // SLATE_BUILTINS_H