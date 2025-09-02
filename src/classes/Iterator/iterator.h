#ifndef SLATE_ITERATOR_CLASS_H
#define SLATE_ITERATOR_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;

// Iterator Class Initialization
void iterator_class_init(vm_t* vm);

// Iterator Factory Function
value_t builtin_iterator(vm_t* vm, int arg_count, value_t* args);

// Iterator Functions (for global usage)
value_t builtin_has_next(vm_t* vm, int arg_count, value_t* args);
value_t builtin_next(vm_t* vm, int arg_count, value_t* args);

// Iterator Instance Methods
value_t builtin_iterator_is_empty(vm_t* vm, int arg_count, value_t* args);
value_t builtin_iterator_to_array(vm_t* vm, int arg_count, value_t* args);

#endif // SLATE_ITERATOR_CLASS_H