#include "buffer.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global Buffer class storage
value_t* global_buffer_class = NULL;

// Initialize Buffer class with prototype and methods
void buffer_class_init(vm_t* vm) {
    // Create the Buffer class with its prototype
    do_object buffer_proto = do_create(NULL);

    // Add methods to Buffer prototype
    value_t buffer_slice_method = make_native(builtin_buffer_method_slice);
    do_set(buffer_proto, "slice", &buffer_slice_method, sizeof(value_t));

    value_t buffer_concat_method = make_native(builtin_buffer_method_concat);
    do_set(buffer_proto, "concat", &buffer_concat_method, sizeof(value_t));

    value_t buffer_to_hex_method = make_native(builtin_buffer_method_to_hex);
    do_set(buffer_proto, "toHex", &buffer_to_hex_method, sizeof(value_t));

    value_t buffer_length_method = make_native(builtin_buffer_method_length);
    do_set(buffer_proto, "length", &buffer_length_method, sizeof(value_t));

    value_t buffer_equals_method = make_native(builtin_buffer_method_equals);
    do_set(buffer_proto, "equals", &buffer_equals_method, sizeof(value_t));

    value_t buffer_to_string_method = make_native(builtin_buffer_method_to_string);
    do_set(buffer_proto, "toString", &buffer_to_string_method, sizeof(value_t));

    value_t buffer_reader_method = make_native(builtin_buffer_method_reader);
    do_set(buffer_proto, "reader", &buffer_reader_method, sizeof(value_t));

    // Create the Buffer class
    value_t buffer_class = make_class("Buffer", buffer_proto);

    // Set the factory function 
    buffer_class.as.class->factory = buffer_factory;

    // Add static methods to the Buffer class
    value_t from_hex_method = make_native(builtin_buffer_from_hex);
    do_set(buffer_class.as.class->properties, "fromHex", &from_hex_method, sizeof(value_t));

    // Store in globals
    do_set(vm->globals, "Buffer", &buffer_class, sizeof(value_t));

    // Store a global reference for use in make_buffer
    static value_t buffer_class_storage;
    buffer_class_storage = vm_retain(buffer_class);
    global_buffer_class = &buffer_class_storage;
}