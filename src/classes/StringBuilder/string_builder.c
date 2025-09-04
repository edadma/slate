#include "string_builder.h"
#include "builtins.h"
#include "dynamic_string.h"
#include "dynamic_object.h"
#include "class_string.h"

// Global StringBuilder class storage
value_t* global_string_builder_class = NULL;

// Initialize StringBuilder class with prototype and methods
void string_builder_class_init(vm_t* vm) {
    // Create the StringBuilder class with its prototype  
    do_object string_builder_proto = do_create(NULL);

    // Add methods to StringBuilder prototype
    value_t sb_append_method = make_native(builtin_string_builder_append);
    do_set(string_builder_proto, "append", &sb_append_method, sizeof(value_t));

    value_t sb_append_char_method = make_native(builtin_string_builder_append_char);
    do_set(string_builder_proto, "appendChar", &sb_append_char_method, sizeof(value_t));

    value_t sb_to_string_method = make_native(builtin_string_builder_to_string);
    do_set(string_builder_proto, "toString", &sb_to_string_method, sizeof(value_t));

    value_t sb_length_method = make_native(builtin_string_builder_length);
    do_set(string_builder_proto, "length", &sb_length_method, sizeof(value_t));

    value_t sb_clear_method = make_native(builtin_string_builder_clear);
    do_set(string_builder_proto, "clear", &sb_clear_method, sizeof(value_t));

    value_t sb_hash_method = make_native(builtin_string_builder_hash);
    do_set(string_builder_proto, "hash", &sb_hash_method, sizeof(value_t));

    value_t sb_equals_method = make_native(builtin_string_builder_equals);
    do_set(string_builder_proto, "equals", &sb_equals_method, sizeof(value_t));

    // Create the StringBuilder class
    value_t string_builder_class = make_class("StringBuilder", string_builder_proto, NULL);

    // Set the factory function
    string_builder_class.as.class->factory = string_builder_factory;

    // Store in globals
    do_set(vm->globals, "StringBuilder", &string_builder_class, sizeof(value_t));

    // Store a global reference for use in make_string_builder
    static value_t string_builder_class_storage;
    string_builder_class_storage = vm_retain(string_builder_class);
    global_string_builder_class = &string_builder_class_storage;
}

// StringBuilder factory function
value_t string_builder_factory(vm_t* vm, int arg_count, value_t* args) {
    // Parse optional initial capacity (first arg if it's an integer)
    size_t initial_capacity = 16; // default capacity
    int string_arg_start = 0;
    
    if (arg_count > 0 && (args[0].type == VAL_INT32 || args[0].type == VAL_BIGINT)) {
        // First argument is capacity
        if (args[0].type == VAL_INT32) {
            if (args[0].as.int32 < 0) {
                runtime_error(vm, "StringBuilder initial capacity cannot be negative: %d", args[0].as.int32);
                return make_null();
            }
            initial_capacity = (size_t)args[0].as.int32;
        } else {
            // For BigInt capacity, we'll just use default for simplicity
            runtime_error(vm, "BigInt capacity not yet supported for StringBuilder");
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
            runtime_error(vm, "StringBuilder() string arguments must be strings, not %s", value_type_name(args[i].type));
            return make_null();
        }
        
        // Append the string to the builder
        ds_builder_append_string(builder, args[i].as.string);
    }
    
    // Create and return the StringBuilder value
    return make_string_builder(builder);
}

// StringBuilder method: append(string) - appends a string to the builder
value_t builtin_string_builder_append(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "append() requires exactly 1 argument (the string to append)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t str_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "append() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
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
value_t builtin_string_builder_append_char(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "appendChar() requires exactly 1 argument (the codepoint)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t codepoint_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "appendChar() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    if (codepoint_val.type != VAL_INT32) {
        runtime_error(vm, "appendChar() requires an integer codepoint, not %s", value_type_name(codepoint_val.type));
        return make_null();
    }
    
    uint32_t codepoint = (uint32_t)codepoint_val.as.int32;
    if (codepoint_val.as.int32 < 0 || codepoint > 0x10FFFF) {
        runtime_error(vm, "Invalid Unicode codepoint: 0x%X", codepoint);
        return make_null();
    }
    
    // Append the codepoint to the builder
    ds_builder_append_char(receiver.as.string_builder, codepoint);
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: toString() - converts builder to string
value_t builtin_string_builder_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "toString() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert builder to string (creates a copy)
    ds_string result = ds_builder_to_string(receiver.as.string_builder);
    return make_string_ds(result);
}

// StringBuilder method: length() - returns the current length
value_t builtin_string_builder_length(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "length() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "length() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Get the current length of the builder
    size_t len = ds_builder_length(receiver.as.string_builder);
    return make_int32((int32_t)len);
}

// StringBuilder method: clear() - clears the builder
value_t builtin_string_builder_clear(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "clear() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "clear() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Clear the builder
    ds_builder_clear(receiver.as.string_builder);
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: hash() - returns hash code of current content
value_t builtin_string_builder_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "hash() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert to string and compute hash of the content using String.hash()
    ds_string content = ds_builder_to_string(receiver.as.string_builder);
    value_t content_string = make_string_ds(content);
    value_t hash_result = builtin_string_hash(vm, 1, &content_string);
    vm_release(content_string);
    
    return hash_result;
}

// StringBuilder method: equals(other) - compares content with another value
value_t builtin_string_builder_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() requires exactly 1 argument");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error(vm, "equals() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // StringBuilder only equals another StringBuilder with same content
    if (other.type != VAL_STRING_BUILDER) {
        return make_boolean(0);
    }
    
    // Compare content by converting both to strings
    ds_string content1 = ds_builder_to_string(receiver.as.string_builder);
    ds_string content2 = ds_builder_to_string(other.as.string_builder);
    
    int equal = strcmp(content1, content2) == 0;
    
    ds_release(&content1);
    ds_release(&content2);
    
    return make_boolean(equal);
}