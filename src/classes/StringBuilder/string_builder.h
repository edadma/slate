#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;
void runtime_error(vm_t* vm, const char* message, ...);
value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args);

// StringBuilder Factory and Methods  
value_t string_builder_factory(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append_char(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_length(vm_t* vm, int arg_count, value_t* args);
value_t builtin_string_builder_clear(vm_t* vm, int arg_count, value_t* args);

// StringBuilder Class Initialization
void string_builder_class_init(vm_t* vm);

#endif // STRING_BUILDER_H