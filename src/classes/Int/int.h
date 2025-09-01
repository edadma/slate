#ifndef SLATE_INT_CLASS_H
#define SLATE_INT_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// Int Class Initialization
void int_class_init(slate_vm* vm);

// Int Factory Function
value_t int_factory(value_t* args, int arg_count);

// Int Instance Methods
value_t builtin_int_to_string(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_set_bit(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_clear_bit(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_toggle_bit(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_get_bit(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_count_bits(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_leading_zeros(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_trailing_zeros(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_is_even(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_is_odd(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_is_prime(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_gcd(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_lcm(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_pow(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_int_factorial(slate_vm* vm, int arg_count, value_t* args);

// Helper Functions
value_t safe_int_multiply(value_t a, value_t b);

#endif // SLATE_INT_CLASS_H