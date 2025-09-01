#ifndef SLATE_ARRAY_CLASS_H
#define SLATE_ARRAY_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;
void runtime_error(const char* message, ...);

// Array Class Initialization
void array_class_init(slate_vm* vm);

// Array Factory Function
value_t array_factory(value_t* args, int arg_count);

// Array Instance Methods
value_t builtin_array_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_push(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_pop(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_is_empty(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_non_empty(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_index_of(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_contains(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_copy(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_slice(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_reverse(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_fill(slate_vm* vm, int arg_count, value_t* args);

// Array Functional Methods
value_t builtin_array_map(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_filter(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_array_flatmap(slate_vm* vm, int arg_count, value_t* args);

// External dependencies from other modules
value_t builtin_iterator(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_ARRAY_CLASS_H