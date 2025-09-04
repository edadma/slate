#ifndef CLASS_FLOAT_H
#define CLASS_FLOAT_H

#include "vm.h"
#include "value.h"

// Float Class Initialization
void float_class_init(vm_t* vm);

// Float Factory Function
value_t float_factory(vm_t* vm, int arg_count, value_t* args);

// Float Instance Methods
value_t builtin_float_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_abs(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_sign(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_is_finite(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_is_integer(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_sqrt(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_floor(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_ceil(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_round(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_sin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_cos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_tan(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_exp(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_ln(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_asin(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_acos(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_atan(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_degrees(vm_t* vm, int arg_count, value_t* args);
value_t builtin_float_radians(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_FLOAT_H