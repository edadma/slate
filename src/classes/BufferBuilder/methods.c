#include "buffer_builder.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include "dynamic_int.h"
#include <stdint.h>

// BufferBuilder instance method: appendUint8(value)
value_t builtin_buffer_builder_append_uint8(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "appendUint8() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t value_val = args[1];

    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "appendUint8() can only be called on BufferBuilder, not %s", value_type_name(receiver.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error(vm, "appendUint8() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 255) {
        runtime_error(vm, "appendUint8() value must be 0-255, got %d", value);
    }

    db_builder builder = receiver.as.builder;
    if (db_builder_append_uint8(builder, (uint8_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    // Return receiver for method chaining
    return vm_retain(receiver);
}

// BufferBuilder instance method: appendUint16LE(value)
value_t builtin_buffer_builder_append_uint16_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "appendUint16LE() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t value_val = args[1];

    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "appendUint16LE() can only be called on BufferBuilder, not %s", value_type_name(receiver.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error(vm, "appendUint16LE() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 65535) {
        runtime_error(vm, "appendUint16LE() value must be 0-65535, got %d", value);
    }

    db_builder builder = receiver.as.builder;
    if (db_builder_append_uint16_le(builder, (uint16_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    // Return receiver for method chaining
    return vm_retain(receiver);
}

// BufferBuilder instance method: appendUint32LE(value)
value_t builtin_buffer_builder_append_uint32_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "appendUint32LE() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t value_val = args[1];

    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "appendUint32LE() can only be called on BufferBuilder, not %s", value_type_name(receiver.type));
    }
    uint32_t value;
    if (value_val.type == VAL_INT32) {
        if (value_val.as.int32 < 0) {
            runtime_error(vm, "appendUint32LE() value must be non-negative, got %d", value_val.as.int32);
        }
        value = (uint32_t)value_val.as.int32;
    } else if (value_val.type == VAL_BIGINT) {
        // Convert BigInt to uint32 with range checking using the fixed di_to_uint32
        if (!di_to_uint32(value_val.as.bigint, &value)) {
            runtime_error(vm, "appendUint32LE() value must be a non-negative integer that fits in uint32 range");
        }
    } else {
        runtime_error(vm, "appendUint32LE() requires an integer value, not %s", value_type_name(value_val.type));
    }

    db_builder builder = receiver.as.builder;
    if (db_builder_append_uint32_le(builder, (uint32_t)value) != 0) {
        runtime_error(vm, "Failed to append to buffer builder");
    }

    // Return receiver for method chaining
    return vm_retain(receiver);
}

// BufferBuilder instance method: appendString(string)
value_t builtin_buffer_builder_append_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "appendString() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t string_val = args[1];

    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "appendString() can only be called on BufferBuilder, not %s", value_type_name(receiver.type));
    }
    if (string_val.type != VAL_STRING) {
        runtime_error(vm, "appendString() requires a string value, not %s", value_type_name(string_val.type));
    }

    const char* str = string_val.as.string;
    if (!str) {
        runtime_error(vm, "appendString() requires a non-null string");
    }

    db_builder builder = receiver.as.builder;
    if (db_builder_append_cstring(builder, str) != 0) {
        runtime_error(vm, "Failed to append string to buffer builder");
    }

    // Return receiver for method chaining
    return vm_retain(receiver);
}

// BufferBuilder instance method: build() or finish()
value_t builtin_buffer_builder_build(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) { // receiver only
        runtime_error(vm, "build() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "build() can only be called on BufferBuilder, not %s", value_type_name(receiver.type));
    }

    db_builder builder = receiver.as.builder;
    db_buffer result = db_builder_finish(&builder);

    // After finish, the builder is invalidated - reference counting handles cleanup

    return make_buffer(result);
}

// BufferBuilder instance method: toString()
value_t builtin_buffer_builder_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments");
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "toString() can only be called on BufferBuilder");
    }

    // Return a simple string representation
    return make_string("[BufferBuilder]");
}

// BufferBuilder instance method: hash()
value_t builtin_buffer_builder_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments");
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "hash() can only be called on BufferBuilder");
    }

    // Use the builder pointer for hash (simple but consistent)
    db_builder builder = receiver.as.builder;
    uint32_t hash = (uint32_t)((uintptr_t)builder * 2654435761U); // Simple hash
    return make_int32((int32_t)hash);
}

// BufferBuilder instance method: equals(other)
value_t builtin_buffer_builder_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument");
    }

    value_t receiver = args[0];
    value_t other = args[1];

    if (receiver.type != VAL_BUFFER_BUILDER) {
        runtime_error(vm, "equals() can only be called on BufferBuilder");
    }

    // BufferBuilders are equal only if they're the exact same object
    if (other.type == VAL_BUFFER_BUILDER) {
        return make_boolean(receiver.as.builder == other.as.builder);
    }

    return make_boolean(false);
}