#include "buffer.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include "dynamic_array.h"
#include <string.h>
#include <stdlib.h>

// Buffer factory function for class instantiation
value_t buffer_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count == 0) {
        runtime_error(vm, "Buffer() requires at least 1 argument");
        return make_null();
    }

    // Single argument: string or array
    value_t arg = args[0];
    db_buffer buf;
    
    if (arg.type == VAL_STRING) {
        // Create buffer from string
        const char* str = arg.as.string;
        size_t len = str ? strlen(str) : 0;
        buf = db_new_with_data(str, len);
        return make_buffer(buf);
    } else if (arg.type == VAL_ARRAY) {
        // Create buffer from array of bytes (same logic as builtin_buffer)
        da_array arr = arg.as.array;
        size_t len = da_length(arr);

        // Convert array elements to bytes
        uint8_t* bytes = malloc(len);
        if (!bytes) {
            runtime_error(vm, "Failed to allocate memory for buffer");
            return make_null();
        }

        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            if (!elem) {
                free(bytes);
                runtime_error(vm, "Invalid array element at index %zu", i);
                return make_null();
            }
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0 || elem->as.int32 > 255) {
                    free(bytes);
                    runtime_error(vm, "Array element %d at index %zu is not a valid byte (0-255)", elem->as.int32, i);
                    return make_null();
                }
                bytes[i] = (uint8_t)elem->as.int32;
            } else {
                free(bytes);
                runtime_error(vm, "Array element at index %zu must be an integer, not %s", i, value_type_name(elem->type));
                return make_null();
            }
        }

        buf = db_new_with_data(bytes, len);
        free(bytes);
        return make_buffer(buf);
    } else {
        runtime_error(vm, "Buffer() argument must be a string or array, not %s", value_type_name(arg.type));
        return make_null();
    }
}

// Buffer.fromHex(hex_string) - Create buffer from hex string
value_t builtin_buffer_from_hex(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "buffer_from_hex() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t hex_val = args[0];
    if (hex_val.type != VAL_STRING) {
        runtime_error(vm, "buffer_from_hex() requires a string argument, not %s", value_type_name(hex_val.type));
    }

    const char* hex_str = hex_val.as.string;
    if (!hex_str) {
        runtime_error(vm, "buffer_from_hex() requires a non-null string");
    }

    size_t len = strlen(hex_str);
    db_buffer buf = db_from_hex(hex_str, len);

    if (!buf) {
        runtime_error(vm, "Invalid hex string");
    }

    return make_buffer(buf);
}