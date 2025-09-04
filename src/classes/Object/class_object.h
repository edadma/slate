#ifndef CLASS_OBJECT_H
#define CLASS_OBJECT_H

#include "vm.h"
#include "value.h"

void initialize_object_class(vm_t* vm);

value_t builtin_object_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_object_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_object_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_object_keys(vm_t* vm, int arg_count, value_t* args);
value_t builtin_object_values(vm_t* vm, int arg_count, value_t* args);
value_t builtin_object_has(vm_t* vm, int arg_count, value_t* args);

#endif