#ifndef SLATE_BUFFER_BUILDER_CLASS_H
#define SLATE_BUFFER_BUILDER_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);

// BufferBuilder Functions
value_t builtin_buffer_builder(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint8(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint16_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint32_le(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_append_cstring(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_builder_finish(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_BUFFER_BUILDER_CLASS_H