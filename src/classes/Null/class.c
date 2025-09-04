#include "class_null.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global Null class storage
value_t* global_null_class = NULL;

// Initialize Null class with prototype and methods
void initialize_null_class(vm_t* vm) {
    // Create the Null class with its prototype
    do_object null_proto = do_create(NULL);

    // Add methods to Null prototype
    value_t null_hash_method = make_native(builtin_null_hash);
    do_set(null_proto, "hash", &null_hash_method, sizeof(value_t));
    
    value_t null_equals_method = make_native(builtin_null_equals);
    do_set(null_proto, "equals", &null_equals_method, sizeof(value_t));

    value_t null_to_string_method = make_native(builtin_null_to_string);
    do_set(null_proto, "toString", &null_to_string_method, sizeof(value_t));

    // Create the Null class
    value_t null_class = make_class("Null", null_proto, NULL);

    // Store in globals
    do_set(vm->globals, "Null", &null_class, sizeof(value_t));

    // Store a global reference for use in make_null
    static value_t null_class_storage;
    null_class_storage = vm_retain(null_class);
    global_null_class = &null_class_storage;
}