#include "string.h"
#include "builtins.h"
#include "dynamic_string.h"

// Global StringBuilder class storage
value_t* global_string_builder_class = NULL;

// StringBuilder factory function
value_t string_builder_factory(value_t* args, int arg_count) {
    // Parse optional initial capacity (first arg if it's an integer)
    size_t initial_capacity = 16; // default capacity
    int string_arg_start = 0;
    
    if (arg_count > 0 && (args[0].type == VAL_INT32 || args[0].type == VAL_BIGINT)) {
        // First argument is capacity
        if (args[0].type == VAL_INT32) {
            if (args[0].as.int32 < 0) {
                runtime_error("StringBuilder initial capacity cannot be negative: %d", args[0].as.int32);
                return make_null();
            }
            initial_capacity = (size_t)args[0].as.int32;
        } else {
            // For BigInt capacity, we'll just use default for simplicity
            runtime_error("BigInt capacity not yet supported for StringBuilder");
            return make_null();
        }
        string_arg_start = 1;
    }
    
    // Create builder with specified capacity
    ds_builder builder = ds_builder_create_with_capacity(initial_capacity);
    
    // Append any string arguments to the initial contents
    for (int i = string_arg_start; i < arg_count; i++) {
        if (args[i].type != VAL_STRING) {
            ds_builder_release(&builder);
            runtime_error("StringBuilder() string arguments must be strings, not %s", value_type_name(args[i].type));
            return make_null();
        }
        
        // Append the string to the builder
        ds_builder_append_string(builder, args[i].as.string);
    }
    
    // Create and return the StringBuilder value
    return make_string_builder(builder);
}

// StringBuilder method: append(string) - appends a string to the builder
value_t builtin_string_builder_append(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("append() requires exactly 1 argument (the string to append)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t str_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("append() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert value to string if it's not already a string
    value_t string_val;
    if (str_val.type == VAL_STRING) {
        string_val = str_val;
    } else {
        // Call toString() on the value to convert it
        string_val = builtin_value_to_string(vm, 1, &str_val);
    }
    
    // Append the string to the builder
    ds_builder_append_string(receiver.as.string_builder, string_val.as.string);
    
    // Release the string if we created it
    if (str_val.type != VAL_STRING) {
        vm_release(string_val);
    }
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: appendChar(codepoint) - appends a Unicode codepoint
value_t builtin_string_builder_append_char(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("appendChar() requires exactly 1 argument (the codepoint)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t codepoint_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("appendChar() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    if (codepoint_val.type != VAL_INT32) {
        runtime_error("appendChar() requires an integer codepoint, not %s", value_type_name(codepoint_val.type));
        return make_null();
    }
    
    uint32_t codepoint = (uint32_t)codepoint_val.as.int32;
    if (codepoint_val.as.int32 < 0 || codepoint > 0x10FFFF) {
        runtime_error("Invalid Unicode codepoint: 0x%X", codepoint);
        return make_null();
    }
    
    // Append the codepoint to the builder
    ds_builder_append_char(receiver.as.string_builder, codepoint);
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: toString() - converts builder to string
value_t builtin_string_builder_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toString() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("toString() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert builder to string (creates a copy)
    ds_string result = ds_builder_to_string(receiver.as.string_builder);
    return make_string_ds(result);
}

// StringBuilder method: length() - returns the current length
value_t builtin_string_builder_length(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("length() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("length() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Get the current length of the builder
    size_t len = ds_builder_length(receiver.as.string_builder);
    return make_int32((int32_t)len);
}

// StringBuilder method: clear() - clears the builder
value_t builtin_string_builder_clear(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("clear() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("clear() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Clear the builder
    ds_builder_clear(receiver.as.string_builder);
    
    // Return the receiver for chaining
    return receiver;
}