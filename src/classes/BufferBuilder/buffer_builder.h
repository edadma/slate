#ifndef SLATE_BUFFER_BUILDER_CLASS_H
#define SLATE_BUFFER_BUILDER_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;
void runtime_error(vm_t* vm, const char* message, ...);

// BufferBuilder Functions
value_t builtin_buffer_builder(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint8(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_append_cstring(vm_t* vm, int arg_count, value_t* args);
value_t builtin_builder_finish(vm_t* vm, int arg_count, value_t* args);

#endif // SLATE_BUFFER_BUILDER_CLASS_H