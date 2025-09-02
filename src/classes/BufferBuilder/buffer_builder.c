#include "buffer_builder.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include <stdint.h>

// buffer_builder(capacity) - Create buffer builder
value_t builtin_buffer_builder(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "buffer_builder() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t capacity_val = args[0];
    if (capacity_val.type != VAL_INT32) {
        runtime_error(vm, "buffer_builder() requires an integer capacity, not %s", value_type_name(capacity_val.type));
    }

    int32_t capacity = capacity_val.as.int32;
    if (capacity < 0) {
        runtime_error(vm, "buffer_builder() capacity must be non-negative");
    }

    // Create reference counted builder directly
    db_builder builder = db_builder_new((size_t)capacity);
    return make_buffer_builder(builder);
}

// builder_append_uint8(builder, value) - Append uint8 to builder
value_t builtin_builder_append_uint8(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "builder_append_uint8() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "builder_append_uint8() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error(vm, "builder_append_uint8() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 255) {
        runtime_error(vm, "builder_append_uint8() value must be 0-255, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint8(builder, (uint8_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint16_le(builder, value) - Append uint16 in little-endian
value_t builtin_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "builder_append_uint16_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "builder_append_uint16_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error(vm, "builder_append_uint16_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 65535) {
        runtime_error(vm, "builder_append_uint16_le() value must be 0-65535, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint16_le(builder, (uint16_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint32_le(builder, value) - Append uint32 in little-endian
value_t builtin_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "builder_append_uint32_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "builder_append_uint32_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error(vm, "builder_append_uint32_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0) {
        runtime_error(vm, "builder_append_uint32_le() value must be non-negative, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint32_le(builder, (uint32_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_cstring(builder, string) - Append string to builder
value_t builtin_builder_append_cstring(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "builder_append_cstring() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t string_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "builder_append_cstring() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (string_val.type != VAL_STRING) {
        runtime_error(vm, "builder_append_cstring() requires a string value, not %s", value_type_name(string_val.type));
    }

    const char* str = string_val.as.string;
    if (!str) {
        runtime_error(vm, "builder_append_cstring() requires a non-null string");
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_cstring(builder, str) != 0) {
        runtime_error(vm, "Failed to append string to buffer builder");
    }

    return make_null();
}

// builder_finish(builder) - Finish building and get buffer
value_t builtin_builder_finish(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "builder_finish() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "builder_finish() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }

    db_builder builder = builder_val.as.builder;
    db_buffer result = db_builder_finish(&builder);

    // After finish, the builder is invalidated - reference counting handles cleanup

    return make_buffer(result);
}