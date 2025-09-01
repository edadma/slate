#include "array.h"
#include "builtins.h"
#include "dynamic_array.h"

// Array method: length()
value_t builtin_array_length(slate_vm* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the array)
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("length() can only be called on arrays");
    }
    
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: push(element)
// Adds element to end of array, returns new length
value_t builtin_array_push(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("push() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("push() can only be called on arrays");
    }
    
    // Add element to array
    da_push(receiver.as.array, &element);
    
    // Return new length
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: pop()
// Removes and returns last element
value_t builtin_array_pop(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("pop() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("pop() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    if (length == 0) {
        return make_null(); // Return null for empty array
    }
    
    // Get last element
    value_t* last_elem = (value_t*)da_get(array, length - 1);
    value_t result = *last_elem; // Copy the value
    result = vm_retain(result); // Retain since we're returning it
    
    // Remove last element
    da_remove(array, length - 1, NULL);
    
    return result;
}

// Array method: isEmpty()
// Returns true if array has no elements
value_t builtin_array_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("isEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(is_empty);
}

// Array method: nonEmpty()
// Returns true if array has at least one element (Scala-inspired)
value_t builtin_array_non_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("nonEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("nonEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(!is_empty);
}

// Array method: indexOf(element)
// Returns first index of element, or -1 if not found
value_t builtin_array_index_of(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("indexOf() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("indexOf() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    // Search for element
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(array, i);
        if (values_equal(*elem, element)) {
            return make_int32((int32_t)i);
        }
    }
    
    return make_int32(-1); // Not found
}

// Array method: contains(element)
// Returns true if array contains element
value_t builtin_array_contains(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("contains() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    // Search for element
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(array, i);
        if (values_equal(*elem, element)) {
            return make_boolean(true);
        }
    }
    
    return make_boolean(false); // Not found
}

// Array method: copy()
// Returns a copy of the array
value_t builtin_array_copy(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("copy() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("copy() can only be called on arrays");
    }
    
    // Create a copy using da_copy
    da_array copy = da_copy(receiver.as.array);
    return make_array(copy);
}

// Array method: slice(start, end?)
// Returns a subset of the array from start to end (exclusive)
value_t builtin_array_slice(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count < 2 || arg_count > 3) {
        runtime_error("slice() takes 1 or 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t start_val = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("slice() can only be called on arrays");
    }
    
    if (start_val.type != VAL_INT32) {
        runtime_error("slice() start index must be an integer");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    int start = start_val.as.int32;
    int end = (int)length; // Default to end of array
    
    // Handle optional end parameter
    if (arg_count == 3) {
        value_t end_val = args[2];
        if (end_val.type != VAL_INT32) {
            runtime_error("slice() end index must be an integer");
        }
        end = end_val.as.int32;
    }
    
    // Handle negative indices (Python-style)
    if (start < 0) start += (int)length;
    if (end < 0) end += (int)length;
    
    // Clamp to valid range
    if (start < 0) start = 0;
    if (end > (int)length) end = (int)length;
    if (start > end) start = end;
    
    // Create slice using da_slice
    da_array slice = da_slice(array, start, end);
    return make_array(slice);
}

// Array method: reverse()
// Reverses the array in-place and returns it
value_t builtin_array_reverse(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reverse() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("reverse() can only be called on arrays");
    }
    
    // Reverse the array in-place
    da_reverse(receiver.as.array);
    
    // Return the array (for chaining)
    return vm_retain(receiver);
}