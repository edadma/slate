#include "buffer_builder.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global BufferBuilder class storage
value_t* global_buffer_builder_class = NULL;

// Initialize BufferBuilder class with prototype and methods
void buffer_builder_class_init(vm_t* vm) {
    // Create the BufferBuilder class with its prototype
    do_object buffer_builder_proto = do_create(NULL);

    // Add instance methods to BufferBuilder prototype
    value_t append_uint8_method = make_native(builtin_buffer_builder_append_uint8);
    do_set(buffer_builder_proto, "appendUint8", &append_uint8_method, sizeof(value_t));

    value_t append_uint16_le_method = make_native(builtin_buffer_builder_append_uint16_le);
    do_set(buffer_builder_proto, "appendUint16LE", &append_uint16_le_method, sizeof(value_t));

    value_t append_uint32_le_method = make_native(builtin_buffer_builder_append_uint32_le);
    do_set(buffer_builder_proto, "appendUint32LE", &append_uint32_le_method, sizeof(value_t));

    value_t append_string_method = make_native(builtin_buffer_builder_append_string);
    do_set(buffer_builder_proto, "appendString", &append_string_method, sizeof(value_t));

    value_t build_method = make_native(builtin_buffer_builder_build);
    do_set(buffer_builder_proto, "build", &build_method, sizeof(value_t));

    // Add standard methods
    value_t to_string_method = make_native(builtin_buffer_builder_to_string);
    do_set(buffer_builder_proto, "toString", &to_string_method, sizeof(value_t));

    value_t hash_method = make_native(builtin_buffer_builder_hash);
    do_set(buffer_builder_proto, "hash", &hash_method, sizeof(value_t));

    value_t equals_method = make_native(builtin_buffer_builder_equals);
    do_set(buffer_builder_proto, "equals", &equals_method, sizeof(value_t));

    // Create the BufferBuilder class (no static methods needed)
    value_t buffer_builder_class = make_class("BufferBuilder", buffer_builder_proto, NULL);

    // Set the factory function to allow BufferBuilder(capacity)
    buffer_builder_class.as.class->factory = buffer_builder_factory;

    // Store in globals
    do_set(vm->globals, "BufferBuilder", &buffer_builder_class, sizeof(value_t));

    // Store a global reference for use in make_buffer_builder
    static value_t buffer_builder_class_storage;
    buffer_builder_class_storage = vm_retain(buffer_builder_class);
    global_buffer_builder_class = &buffer_builder_class_storage;
}