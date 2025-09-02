#include "array.h"
#include "builtins.h"
#include "dynamic_array.h"

// Array method: length()
value_t builtin_array_length(vm_t* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the array)
    if (arg_count != 1) {
        runtime_error(vm, "length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "length() can only be called on arrays");
    }
    
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: push(element)
// Adds element to end of array, returns new length
value_t builtin_array_push(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "push() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "push() can only be called on arrays");
    }
    
    // Add element to array
    da_push(receiver.as.array, &element);
    
    // Return new length
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: pop()
// Removes and returns last element
value_t builtin_array_pop(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "pop() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "pop() can only be called on arrays");
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
value_t builtin_array_is_empty(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "isEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(is_empty);
}

// Array method: nonEmpty()
// Returns true if array has at least one element (Scala-inspired)
value_t builtin_array_non_empty(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "nonEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "nonEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(!is_empty);
}

// Array method: indexOf(element)
// Returns first index of element, or -1 if not found
value_t builtin_array_index_of(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "indexOf() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "indexOf() can only be called on arrays");
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
value_t builtin_array_contains(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error(vm, "contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "contains() can only be called on arrays");
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
value_t builtin_array_copy(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "copy() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "copy() can only be called on arrays");
    }
    
    // Create a copy using da_copy
    da_array copy = da_copy(receiver.as.array);
    return make_array(copy);
}

// Array method: slice(start, end?)
// Returns a subset of the array from start to end (exclusive)
value_t builtin_array_slice(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 2 || arg_count > 3) {
        runtime_error(vm, "slice() takes 1 or 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t start_val = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "slice() can only be called on arrays");
    }
    
    if (start_val.type != VAL_INT32) {
        runtime_error(vm, "slice() start index must be an integer");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    int start = start_val.as.int32;
    int end = (int)length; // Default to end of array
    
    // Handle optional end parameter
    if (arg_count == 3) {
        value_t end_val = args[2];
        if (end_val.type != VAL_INT32) {
            runtime_error(vm, "slice() end index must be an integer");
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
value_t builtin_array_reverse(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reverse() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "reverse() can only be called on arrays");
    }
    
    // Reverse the array in-place
    da_reverse(receiver.as.array);
    
    // Return the array (for chaining)
    return vm_retain(receiver);
}

// Array method: fill(n, f)
// Creates an array of length n, calling function f to generate each element
value_t builtin_array_fill(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error(vm, "fill() takes exactly 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t n_val = args[1];
    value_t f_val = args[2];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "fill() can only be called on arrays");
    }
    
    if (n_val.type != VAL_INT32) {
        runtime_error(vm, "fill() first argument must be an int32");
    }
    
    int32_t n = n_val.as.int32;
    if (n < 0) {
        runtime_error(vm, "fill() size must be non-negative (%d given)", n);
    }
    
    // Create new array with n elements
    da_array arr = da_new(sizeof(value_t));
    
    // If n is 0, return empty array without checking function
    if (n == 0) {
        return make_array(arr);
    }
    
    if (f_val.type != VAL_CLOSURE) {
        runtime_error(vm, "fill() second argument must be a function");
    }
    
    // Call function f for each element
    for (int32_t i = 0; i < n; i++) {
        // Call function f with no arguments
        value_t element = vm_call_slate_function_from_c(vm, f_val, 0, NULL);
        
        // Add to array (vm_call_function handles retain/release internally)
        da_push(arr, &element);
    }
    
    return make_array(arr);
}