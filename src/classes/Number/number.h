#ifndef CLASS_NUMBER_H
#define CLASS_NUMBER_H

#include "vm.h"
#include "value.h"

// Number Class Initialization
void number_class_init(vm_t* vm);

// Number Class Methods (common to all numeric types)
value_t builtin_number_test(vm_t* vm, int arg_count, value_t* args);
value_t builtin_number_abs(vm_t* vm, int arg_count, value_t* args);
value_t builtin_number_sign(vm_t* vm, int arg_count, value_t* args);
value_t builtin_number_is_finite(vm_t* vm, int arg_count, value_t* args);
value_t builtin_number_is_integer(vm_t* vm, int arg_count, value_t* args);
value_t builtin_number_to_precision(vm_t* vm, int arg_count, value_t* args);

// Helper function to check if a value is any numeric type
int is_numeric_value(value_t value);

#endif // CLASS_NUMBER_H