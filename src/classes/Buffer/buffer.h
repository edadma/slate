#ifndef CLASS_BUFFER_H
#define CLASS_BUFFER_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// Buffer Class Initialization
void buffer_class_init(vm_t* vm);

// Buffer Factory Function (forward declaration)
value_t buffer_factory(vm_t* vm, int arg_count, value_t* args);

// Buffer Instance Methods
value_t builtin_buffer_method_slice(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_concat(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_hex(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_reader(vm_t* vm, int arg_count, value_t* args);

// Buffer Static Methods
value_t builtin_buffer_from_hex(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_BUFFER_H