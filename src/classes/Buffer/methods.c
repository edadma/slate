#include "buffer.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include "dynamic_string.h"
#include "buffer_reader.h"
#include <string.h>
#include <stdlib.h>

// Buffer method: slice(offset, length)
// Returns a new buffer containing a slice of the original
value_t builtin_buffer_method_slice(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error(vm, "slice() takes exactly 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t offset_val = args[1];
    value_t length_val = args[2];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "slice() can only be called on buffers");
    }
    if (offset_val.type != VAL_INT32) {
        runtime_error(vm, "slice() offset must be an integer, not %s", value_type_name(offset_val.type));
    }
    if (length_val.type != VAL_INT32) {
        runtime_error(vm, "slice() length must be an integer, not %s", value_type_name(length_val.type));
    }
    
    db_buffer buf = receiver.as.buffer;
    int32_t offset = offset_val.as.int32;
    int32_t length = length_val.as.int32;
    
    if (offset < 0 || length < 0) {
        runtime_error(vm, "slice() offset and length must be non-negative");
    }
    
    db_buffer slice = db_slice(buf, (size_t)offset, (size_t)length);
    if (!slice) {
        runtime_error(vm, "Invalid buffer slice bounds");
    }
    
    return make_buffer(slice);
}

// Buffer method: concat(other)
// Returns a new buffer with the contents of this buffer and the other buffer
value_t builtin_buffer_method_concat(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "concat() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other_val = args[1];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "concat() can only be called on buffers");
    }
    if (other_val.type != VAL_BUFFER) {
        runtime_error(vm, "concat() argument must be a buffer, not %s", value_type_name(other_val.type));
    }
    
    db_buffer result = db_concat(receiver.as.buffer, other_val.as.buffer);
    return make_buffer(result);
}

// Buffer method: toHex()
// Returns a hex string representation of the buffer
value_t builtin_buffer_method_to_hex(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toHex() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "toHex() can only be called on buffers");
    }
    
    db_buffer hex_buf = db_to_hex(receiver.as.buffer, false); // lowercase
    
    // Create null-terminated string from hex buffer
    size_t hex_len = db_size(hex_buf);
    char* null_term_hex = malloc(hex_len + 1);
    if (!null_term_hex) {
        db_release(&hex_buf);
        runtime_error(vm, "Failed to allocate memory for hex string");
    }
    
    memcpy(null_term_hex, hex_buf, hex_len);
    null_term_hex[hex_len] = '\0';
    
    ds_string hex_str = ds_new(null_term_hex);
    
    free(null_term_hex);
    db_release(&hex_buf);
    
    return make_string_ds(hex_str);
}

// Buffer method: length()
// Returns the length of the buffer in bytes
value_t builtin_buffer_method_length(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "length() can only be called on buffers");
    }
    
    size_t size = db_size(receiver.as.buffer);
    return make_int32((int32_t)size);
}

// Buffer method: equals(other)
// Returns true if the buffer contents are equal to another buffer
value_t builtin_buffer_method_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other_val = args[1];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "equals() can only be called on buffers");
    }
    if (other_val.type != VAL_BUFFER) {
        runtime_error(vm, "equals() argument must be a buffer, not %s", value_type_name(other_val.type));
    }
    
    bool equal = db_equals(receiver.as.buffer, other_val.as.buffer);
    return make_boolean(equal);
}

// Buffer method: toString()
// Returns the buffer contents as a string (assuming UTF-8 encoding)
value_t builtin_buffer_method_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "toString() can only be called on buffers");
    }
    
    db_buffer buf = receiver.as.buffer;
    size_t size = db_size(buf);
    
    // Create null-terminated string from buffer
    char* null_term_str = malloc(size + 1);
    if (!null_term_str) {
        runtime_error(vm, "Failed to allocate memory for string");
    }
    
    memcpy(null_term_str, buf, size);
    null_term_str[size] = '\0';
    
    ds_string str = ds_new(null_term_str);
    
    free(null_term_str);
    
    return make_string_ds(str);
}

// Buffer method: reader()
// Returns a buffer reader for this buffer
value_t builtin_buffer_method_reader(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error(vm, "reader() can only be called on buffers");
    }
    
    // Use BufferReader factory to create a proper BufferReader class instance
    return buffer_reader_factory(vm, 1, &receiver);
}