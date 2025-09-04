#ifndef CLASS_BUFFER_BUILDER_H
#define CLASS_BUFFER_BUILDER_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// Global BufferBuilder class reference
extern value_t* global_buffer_builder_class;

// Class initialization
void buffer_builder_class_init(vm_t* vm);

// Factory function
value_t buffer_builder_factory(vm_t* vm, int arg_count, value_t* args);

// Instance methods
value_t builtin_buffer_builder_append_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_append_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_build(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_hash(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_builder_equals(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_BUFFER_BUILDER_H