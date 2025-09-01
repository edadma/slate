#ifndef SLATE_RANGE_CLASS_H
#define SLATE_RANGE_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// Range Class Initialization
void range_class_init(slate_vm* vm);

// Range Instance Methods
value_t builtin_range_start(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_end(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_is_exclusive(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_is_empty(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_length(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_contains(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_to_array(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_reverse(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_range_equals(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_RANGE_CLASS_H