#ifndef CLASS_RANGE_H
#define CLASS_RANGE_H

#include "vm.h"
#include "value.h"

// Range Class Initialization
void range_class_init(vm_t* vm);

// Range Instance Methods
value_t builtin_range_start(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_end(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_is_exclusive(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_contains(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_to_array(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_reverse(vm_t* vm, int arg_count, value_t* args);
value_t builtin_range_equals(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_RANGE_H