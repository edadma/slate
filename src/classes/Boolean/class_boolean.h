#ifndef CLASS_BOOLEAN_H
#define CLASS_BOOLEAN_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// Boolean Factory Function
value_t boolean_factory(vm_t* vm, int arg_count, value_t* args);

// Boolean Instance Methods
value_t builtin_boolean_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_and(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_or(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_not(vm_t* vm, int arg_count, value_t* args);
value_t builtin_boolean_xor(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_BOOLEAN_H