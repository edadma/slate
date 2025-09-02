#ifndef CLASS_BUFFER_BUILDER_H
#define CLASS_BUFFER_BUILDER_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// BufferBuilder Functions
value_t builtin_buffer_builder(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_cstring(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_finish(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_BUFFER_BUILDER_H