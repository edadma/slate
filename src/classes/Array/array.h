#ifndef CLASS_ARRAY_H
#define CLASS_ARRAY_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// Array Class Initialization
void array_class_init(vm_t* vm);

// Array Factory Function
value_t array_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);

// Array Instance Methods
value_t builtin_array_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_equals(vm_t* vm, int arg_count, value_t* args);
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
value_t builtin_array_fill(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_toString(vm_t* vm, int arg_count, value_t* args);

// Array Functional Methods
value_t builtin_array_map(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_filter(vm_t* vm, int arg_count, value_t* args);
value_t builtin_array_flatmap(vm_t* vm, int arg_count, value_t* args);

// External dependencies from other modules
value_t builtin_iterator(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_ARRAY_H