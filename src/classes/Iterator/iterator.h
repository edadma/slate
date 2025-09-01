#ifndef SLATE_ITERATOR_CLASS_H
#define SLATE_ITERATOR_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// Iterator Class Initialization
void iterator_class_init(slate_vm* vm);

// Iterator Factory Function
value_t builtin_iterator(slate_vm* vm, int arg_count, value_t* args);

// Iterator Functions (for global usage)
value_t builtin_has_next(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_next(slate_vm* vm, int arg_count, value_t* args);

// Iterator Instance Methods
value_t builtin_iterator_is_empty(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_iterator_to_array(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_ITERATOR_CLASS_H