#ifndef SLATE_BUFFER_CLASS_H
#define SLATE_BUFFER_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);

// Buffer Class Initialization
void buffer_class_init(slate_vm* vm);

// Buffer Factory Function (forward declaration)
value_t buffer_factory(value_t* args, int arg_count);

// Buffer Instance Methods
value_t builtin_buffer_method_slice(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_concat(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_hex(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_equals(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_string(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_reader(slate_vm* vm, int arg_count, value_t* args);

// Buffer Static Methods
value_t builtin_buffer_from_hex(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_BUFFER_CLASS_H