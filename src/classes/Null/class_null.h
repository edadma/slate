#ifndef CLASS_NULL_H
#define CLASS_NULL_H

#include "vm.h"
#include "value.h"

void initialize_null_class(vm_t* vm);

value_t builtin_null_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_null_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_null_to_string(vm_t* vm, int arg_count, value_t* args);

#endif