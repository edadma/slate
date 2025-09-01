#include "array.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global Array class storage
value_t* global_array_class = NULL;

// Initialize Array class with prototype and methods
void array_class_init(slate_vm* vm) {
    // Create the Array class with its prototype
    do_object array_proto = do_create(NULL);

    // Add methods to Array prototype
    value_t array_length_method = make_native(builtin_array_length);
    do_set(array_proto, "length", &array_length_method, sizeof(value_t));

    value_t array_push_method = make_native(builtin_array_push);
    do_set(array_proto, "push", &array_push_method, sizeof(value_t));

    value_t array_pop_method = make_native(builtin_array_pop);
    do_set(array_proto, "pop", &array_pop_method, sizeof(value_t));

    value_t array_is_empty_method = make_native(builtin_array_is_empty);
    do_set(array_proto, "isEmpty", &array_is_empty_method, sizeof(value_t));

    value_t array_non_empty_method = make_native(builtin_array_non_empty);
    do_set(array_proto, "nonEmpty", &array_non_empty_method, sizeof(value_t));

    value_t array_index_of_method = make_native(builtin_array_index_of);
    do_set(array_proto, "indexOf", &array_index_of_method, sizeof(value_t));

    value_t array_contains_method = make_native(builtin_array_contains);
    do_set(array_proto, "contains", &array_contains_method, sizeof(value_t));

    value_t array_iterator_method = make_native(builtin_iterator);
    do_set(array_proto, "iterator", &array_iterator_method, sizeof(value_t));

    value_t array_copy_method = make_native(builtin_array_copy);
    do_set(array_proto, "copy", &array_copy_method, sizeof(value_t));

    value_t array_slice_method = make_native(builtin_array_slice);
    do_set(array_proto, "slice", &array_slice_method, sizeof(value_t));

    value_t array_reverse_method = make_native(builtin_array_reverse);
    do_set(array_proto, "reverse", &array_reverse_method, sizeof(value_t));

    // Functional programming methods
    value_t array_map_method = make_native(builtin_array_map);
    do_set(array_proto, "map", &array_map_method, sizeof(value_t));

    value_t array_filter_method = make_native(builtin_array_filter);
    do_set(array_proto, "filter", &array_filter_method, sizeof(value_t));

    value_t array_flatmap_method = make_native(builtin_array_flatmap);
    do_set(array_proto, "flatMap", &array_flatmap_method, sizeof(value_t));

    // Create the Array class
    value_t array_class = make_class("Array", array_proto);

    // Store in globals
    do_set(vm->globals, "Array", &array_class, sizeof(value_t));

    // Store a global reference for use in make_array
    static value_t array_class_storage;
    array_class_storage = vm_retain(array_class);
    global_array_class = &array_class_storage;
}