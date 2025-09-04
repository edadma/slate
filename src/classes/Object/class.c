#include "class_object.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global Object class storage
value_t* global_object_class = NULL;

// Initialize Object class with prototype and methods
void initialize_object_class(vm_t* vm) {
    // Create the Object class with its prototype
    do_object object_proto = do_create(NULL);

    // Add methods to Object prototype
    value_t object_hash_method = make_native(builtin_object_hash);
    do_set(object_proto, "hash", &object_hash_method, sizeof(value_t));
    
    value_t object_equals_method = make_native(builtin_object_equals);
    do_set(object_proto, "equals", &object_equals_method, sizeof(value_t));

    value_t object_to_string_method = make_native(builtin_object_to_string);
    do_set(object_proto, "toString", &object_to_string_method, sizeof(value_t));

    value_t object_keys_method = make_native(builtin_object_keys);
    do_set(object_proto, "keys", &object_keys_method, sizeof(value_t));

    value_t object_values_method = make_native(builtin_object_values);
    do_set(object_proto, "values", &object_values_method, sizeof(value_t));

    value_t object_has_method = make_native(builtin_object_has);
    do_set(object_proto, "has", &object_has_method, sizeof(value_t));

    // Create the Object class
    value_t object_class = make_class("Object", object_proto);

    // Store in globals
    do_set(vm->globals, "Object", &object_class, sizeof(value_t));

    // Store a global reference for use in make_object
    static value_t object_class_storage;
    object_class_storage = vm_retain(object_class);
    global_object_class = &object_class_storage;
}