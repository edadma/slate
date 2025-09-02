#ifndef SLATE_BUFFER_READER_CLASS_H
#define SLATE_BUFFER_READER_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;
void runtime_error(vm_t* vm, const char* message, ...);

// BufferReader class initialization
void buffer_reader_class_init(vm_t* vm);

// BufferReader factory and instance methods
value_t buffer_reader_factory(vm_t* vm, int arg_count, value_t* args);
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

#endif // SLATE_BUFFER_READER_CLASS_H