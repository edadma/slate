#ifndef SLATE_BUFFER_READER_CLASS_H
#define SLATE_BUFFER_READER_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);

// BufferReader Functions
value_t builtin_buffer_reader(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint8(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint16_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_read_uint32_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_position(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_reader_remaining(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_BUFFER_READER_CLASS_H