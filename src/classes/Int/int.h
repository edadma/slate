#ifndef SLATE_INT_CLASS_H
#define SLATE_INT_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;

// Int Class Initialization
void int_class_init(vm_t* vm);

// Int Factory Function
value_t int_factory(vm_t* vm, int arg_count, value_t* args);

// Int Instance Methods
value_t builtin_int_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_set_bit(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_clear_bit(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_toggle_bit(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_get_bit(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_count_bits(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_leading_zeros(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_trailing_zeros(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_is_even(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_is_odd(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_is_prime(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_gcd(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_lcm(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_pow(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_factorial(vm_t* vm, int arg_count, value_t* args);

// Helper Functions
value_t safe_int_multiply(value_t a, value_t b);

#endif // SLATE_INT_CLASS_H