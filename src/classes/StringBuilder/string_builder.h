#ifndef CLASS_STRING_BUILDER_H
#define CLASS_STRING_BUILDER_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args);

// StringBuilder Factory and Methods  
value_t string_builder_factory(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append_char(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_clear(vm_t* vm, int arg_count, value_t* args);

// StringBuilder Class Initialization
void string_builder_class_init(vm_t* vm);

#endif // CLASS_STRING_BUILDER_H