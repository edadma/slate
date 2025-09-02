#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);
value_t builtin_value_to_string(slate_vm* vm, int arg_count, value_t* args);

// StringBuilder Factory and Methods  
value_t string_builder_factory(value_t* args, int arg_count);
value_t builtin_string_builder_append(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append_char(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_to_string(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_clear(slate_vm* vm, int arg_count, value_t* args);

// StringBuilder Class Initialization
void string_builder_class_init(slate_vm* vm);

#endif // STRING_BUILDER_H