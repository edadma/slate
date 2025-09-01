#include "string.h"
#include "builtins.h"
#include "dynamic_string.h"

// String method: length
// This will be used as a method on the String prototype
value_t builtin_string_length(slate_vm* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the string)
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("length() can only be called on strings");
    }

    // Get the length of the string
    size_t length = ds_length(receiver.as.string);

    // Return as int32 if it fits, otherwise as number
    if (length <= INT32_MAX) {
        return make_int32((int32_t)length);
    } else {
        return make_number((double)length);
    }
}

// String method: substring(start, length)
value_t builtin_string_substring(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error("substring() takes exactly 2 arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t start_val = args[1];
    value_t length_val = args[2];

    if (receiver.type != VAL_STRING) {
        runtime_error("substring() can only be called on strings");
    }
    if (!is_int(start_val) || !is_int(length_val)) {
        runtime_error("substring() arguments must be integers");
    }

    int start_int = value_to_int(start_val);
    int length_int = value_to_int(length_val);
    
    if (start_int < 0 || length_int < 0) {
        runtime_error("substring() arguments must be non-negative");
    }

    size_t start = (size_t)start_int;
    size_t length = (size_t)length_int;

    // Create substring using dynamic_string
    ds_string result = ds_substring(receiver.as.string, start, length);
    
    return make_string_ds(result);
}

// String method: toUpper()
value_t builtin_string_to_upper(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toUpper() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("toUpper() can only be called on strings");
    }

    ds_string result = ds_to_upper(receiver.as.string);
    return make_string_ds(result);
}

// String method: toLower()
value_t builtin_string_to_lower(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toLower() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("toLower() can only be called on strings");
    }

    ds_string result = ds_to_lower(receiver.as.string);
    return make_string_ds(result);
}

// String method: trim()
value_t builtin_string_trim(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("trim() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("trim() can only be called on strings");
    }

    ds_string result = ds_trim(receiver.as.string);
    return make_string_ds(result);
}

// String method: startsWith(prefix)
value_t builtin_string_starts_with(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("startsWith() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t prefix_val = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("startsWith() can only be called on strings");
    }
    if (prefix_val.type != VAL_STRING) {
        runtime_error("startsWith() argument must be a string");
    }

    bool result = ds_starts_with(receiver.as.string, prefix_val.as.string);
    return make_boolean(result);
}

// String method: endsWith(suffix)
value_t builtin_string_ends_with(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("endsWith() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t suffix_val = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("endsWith() can only be called on strings");
    }
    if (suffix_val.type != VAL_STRING) {
        runtime_error("endsWith() argument must be a string");
    }

    bool result = ds_ends_with(receiver.as.string, suffix_val.as.string);
    return make_boolean(result);
}

// String method: contains(substring)
value_t builtin_string_contains(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t substring_val = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("contains() can only be called on strings");
    }
    if (substring_val.type != VAL_STRING) {
        runtime_error("contains() argument must be a string");
    }

    bool result = ds_contains(receiver.as.string, substring_val.as.string);
    return make_boolean(result);
}

// String method: replace(old, new)
value_t builtin_string_replace(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error("replace() takes exactly 2 arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t old_val = args[1];
    value_t new_val = args[2];

    if (receiver.type != VAL_STRING) {
        runtime_error("replace() can only be called on strings");
    }
    if (old_val.type != VAL_STRING || new_val.type != VAL_STRING) {
        runtime_error("replace() arguments must be strings");
    }

    ds_string result = ds_replace(receiver.as.string, old_val.as.string, new_val.as.string);
    return make_string_ds(result);
}

// String method: indexOf(substring)
value_t builtin_string_index_of(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("indexOf() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t substring_val = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("indexOf() can only be called on strings");
    }
    if (substring_val.type != VAL_STRING) {
        runtime_error("indexOf() argument must be a string");
    }

    int result = ds_find(receiver.as.string, substring_val.as.string);
    
    if (result == -1) {
        return make_int32(-1);
    } else {
        return make_int32(result);
    }
}

// String method: isEmpty()
value_t builtin_string_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("isEmpty() can only be called on strings");
    }

    bool result = ds_is_empty(receiver.as.string);
    return make_boolean(result);
}

// String method: nonEmpty()  
value_t builtin_string_non_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("nonEmpty() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("nonEmpty() can only be called on strings");
    }

    bool result = !ds_is_empty(receiver.as.string);
    return make_boolean(result);
}