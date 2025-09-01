#ifndef SLATE_STRING_CLASS_H
#define SLATE_STRING_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);
value_t builtin_value_to_string(slate_vm* vm, int arg_count, value_t* args);

// String Factory Function
value_t string_factory(value_t* args, int arg_count);

// String Instance Methods
value_t builtin_string_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_substring(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_to_upper(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_to_lower(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_trim(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_starts_with(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_ends_with(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_contains(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_replace(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_index_of(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_is_empty(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_non_empty(slate_vm* vm, int arg_count, value_t* args);

// StringBuilder Factory and Methods  
value_t string_builder_factory(value_t* args, int arg_count);
value_t builtin_string_builder_append(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_append_char(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_to_string(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_string_builder_clear(slate_vm* vm, int arg_count, value_t* args);

// StringBuilder Class Initialization
void string_builder_class_init(slate_vm* vm);

#endif // SLATE_STRING_CLASS_H