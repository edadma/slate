#ifndef CLASS_BUFFER_READER_H
#define CLASS_BUFFER_READER_H

#include "vm.h"
#include "value.h"
#include "runtime_error.h"

// BufferReader class initialization
void buffer_reader_class_init(vm_t* vm);

// BufferReader factory and instance methods
value_t buffer_reader_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t builtin_buffer_reader_read_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_reader_read_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_reader_read_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_reader_position(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_reader_remaining(vm_t* vm, int arg_count, value_t* args);

// Legacy function support (for backward compatibility)
value_t builtin_buffer_reader(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_position(vm_t* vm, int arg_count, value_t* args);
value_t builtin_reader_remaining(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_BUFFER_READER_H