#ifndef CLASS_INT_H
#define CLASS_INT_H

#include "vm.h"
#include "value.h"

// Int Class Initialization
void int_class_init(vm_t* vm);

// Int Factory Function
value_t int_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);

// Int Instance Methods
value_t builtin_int_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_equals(vm_t* vm, int arg_count, value_t* args);
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

// Number interface methods for integers
value_t builtin_int_abs(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_sign(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_is_finite(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_is_integer(vm_t* vm, int arg_count, value_t* args);

// Math methods for integers
value_t builtin_int_sqrt(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_sin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_cos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_tan(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_exp(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_ln(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_asin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_acos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_atan(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_degrees(vm_t* vm, int arg_count, value_t* args);
value_t builtin_int_radians(vm_t* vm, int arg_count, value_t* args);

// Helper Functions
value_t safe_int_multiply(value_t a, value_t b);

#endif // CLASS_INT_H