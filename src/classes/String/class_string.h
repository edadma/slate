#ifndef CLASS_STRING_H
#define CLASS_STRING_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args);

// String Factory Function
value_t string_factory(vm_t* vm, int arg_count, value_t* args);

// String Instance Methods
value_t builtin_string_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_substring(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_to_upper(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_to_lower(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_trim(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_starts_with(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_ends_with(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_contains(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_replace(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_index_of(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_non_empty(vm_t* vm, int arg_count, value_t* args);


#endif // CLASS_STRING_H