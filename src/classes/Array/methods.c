#include "array.h"
#include "builtins.h"
#include "dynamic_array.h"
#include <stdint.h>

// Using centralized call_equals_method from vm/utilities.c

// FNV-1a hash constants for 32-bit
#define FNV_32_PRIME 0x01000193
#define FNV_32_OFFSET_BASIS 0x811c9dc5

// Forward declaration for hashing values
static uint32_t hash_value_simple(value_t value);

// Array method: hash()
value_t builtin_array_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "hash() can only be called on arrays");
    }
    
    // Combine hash codes of all elements
    uint32_t hash = FNV_32_OFFSET_BASIS;
    size_t length = da_length(receiver.as.array);
    
    for (size_t i = 0; i < length; i++) {
        value_t* element = (value_t*)da_get(receiver.as.array, i);
        if (element) {
            uint32_t element_hash = hash_value_simple(*element);
            hash ^= element_hash;
            hash *= FNV_32_PRIME;
        }
    }
    
    // Mix in the length to distinguish arrays with same elements but different order
    hash ^= (uint32_t)length;
    hash *= FNV_32_PRIME;
    
    return make_int32((int32_t)hash);
}

// Simple hash function for array elements
static uint32_t hash_value_simple(value_t value) {
    switch (value.type) {
    case VAL_NULL:
        return 0;
    case VAL_UNDEFINED:
        return 0x1000000;
    case VAL_BOOLEAN:
        return value.as.boolean ? 1 : 0;
    case VAL_INT32:
        return (uint32_t)value.as.int32;
    case VAL_FLOAT32: {
        union { float f; uint32_t u; } converter;
        converter.f = value.as.float32;
        return converter.u;
    }
    case VAL_FLOAT64: {
        union { double d; uint64_t u; } converter;
        converter.d = value.as.float64;
        return (uint32_t)(converter.u ^ (converter.u >> 32));
    }
    case VAL_STRING: {
        uint32_t hash = FNV_32_OFFSET_BASIS;
        if (value.as.string) {
            for (const char* p = value.as.string; *p; p++) {
                hash ^= (uint8_t)*p;
                hash *= FNV_32_PRIME;
            }
        }
        return hash;
    }
    default:
        // For complex types, use pointer identity
        return (uint32_t)(uintptr_t)&value;
    }
}

// Array method: equals(other) - Deep equality comparison for arrays
value_t builtin_array_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "equals() can only be called on arrays");
    }
    
    // Only arrays can be equal to arrays
    if (other.type != VAL_ARRAY) {
        return make_boolean(0);
    }
    
    // Identity equality check
    if (receiver.as.array == other.as.array) {
        return make_boolean(1);
    }
    
    // Handle null arrays
    if (receiver.as.array == NULL && other.as.array == NULL) {
        return make_boolean(1);
    }
    if (receiver.as.array == NULL || other.as.array == NULL) {
        return make_boolean(0);
    }
    
    // Length comparison
    size_t len1 = da_length(receiver.as.array);
    size_t len2 = da_length(other.as.array);
    if (len1 != len2) {
        return make_boolean(0);
    }
    
    // Element-wise deep comparison
    for (size_t i = 0; i < len1; i++) {
        value_t* elem1 = (value_t*)da_get(receiver.as.array, i);
        value_t* elem2 = (value_t*)da_get(other.as.array, i);
        
        if (elem1 == NULL && elem2 == NULL) continue;
        if (elem1 == NULL || elem2 == NULL) return make_boolean(0);
        
        // Use .equals() method for deep comparison
        if (!call_equals_method(vm, *elem1, *elem2)) {
            return make_boolean(0);
        }
    }
    
    return make_boolean(1);
}

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
        if (call_equals_method(vm, *elem, element)) {
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
        if (call_equals_method(vm, *elem, element)) {
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

// Array static method: fill(n, f)
// Creates an array of length n, calling function f to generate each element
value_t builtin_array_fill(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // n, f args (no receiver for static method)
        runtime_error(vm, "Array.fill() takes exactly 2 arguments (%d given)", arg_count);
    }
    
    value_t n_val = args[0];
    value_t f_val = args[1];
    
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