#include "array.h"
#include "builtins.h"
#include "dynamic_array.h"

// Array method: map(function)
// Returns a new array with the function applied to each element
// JavaScript-style: function receives (element, index, array) as arguments
value_t builtin_array_map(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("map() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t mapper = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("map() can only be called on arrays");
    }
    
    // Check if mapper is callable
    if (mapper.type != VAL_CLOSURE && mapper.type != VAL_NATIVE && mapper.type != VAL_FUNCTION) {
        runtime_error("map() requires a function as argument");
    }
    
    da_array input_array = receiver.as.array;
    size_t length = da_length(input_array);
    
    // Create result array
    da_array result_array = da_new(sizeof(value_t));
    
    // Apply function to each element
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(input_array, i);
        if (!elem) continue;
        
        // Prepare arguments for callback: (element, index, array)
        value_t callback_args[3] = {
            *elem,                      // element
            make_int32((int32_t)i),     // index
            receiver                    // array
        };
        
        value_t mapped_value = vm_call_function(vm, mapper, 3, callback_args);
        
        // Add mapped value to result array  
        da_push(result_array, &mapped_value);
    }
    
    return make_array(result_array);
}

// Array method: filter(predicate)
// Returns a new array with only elements where predicate returns true
// JavaScript-style: function receives (element, index, array) as arguments
value_t builtin_array_filter(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("filter() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t predicate = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("filter() can only be called on arrays");
    }
    
    // Check if predicate is callable
    if (predicate.type != VAL_CLOSURE && predicate.type != VAL_NATIVE && predicate.type != VAL_FUNCTION) {
        runtime_error("filter() requires a function as argument");
    }
    
    da_array input_array = receiver.as.array;
    size_t length = da_length(input_array);
    
    // Create result array
    da_array result_array = da_new(sizeof(value_t));
    
    // Filter elements
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(input_array, i);
        if (!elem) continue;
        
        // Prepare arguments for callback: (element, index, array)
        value_t callback_args[3] = {
            *elem,                      // element
            make_int32((int32_t)i),     // index
            receiver                    // array
        };
        
        value_t result = vm_call_function(vm, predicate, 3, callback_args);
        
        // Check if result is truthy
        if (is_truthy(result)) {
            // Retain the element and add to result
            value_t retained = vm_retain(*elem);
            da_push(result_array, &retained);
        }
    }
    
    return make_array(result_array);
}

// Array method: flatMap(function)
// Maps each element using a mapping function, then flattens the result by one level
// JavaScript-style: function receives (element, index, array) as arguments
value_t builtin_array_flatmap(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("flatMap() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t mapper = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("flatMap() can only be called on arrays");
    }
    
    // Check if mapper is callable
    if (mapper.type != VAL_CLOSURE && mapper.type != VAL_NATIVE && mapper.type != VAL_FUNCTION) {
        runtime_error("flatMap() requires a function as argument");
    }
    
    da_array input_array = receiver.as.array;
    size_t length = da_length(input_array);
    
    // Create result array
    da_array result_array = da_new(sizeof(value_t));
    
    // Apply function to each element and flatten
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(input_array, i);
        if (!elem) continue;
        
        // Prepare arguments for callback: (element, index, array)
        value_t callback_args[3] = {
            *elem,                      // element
            make_int32((int32_t)i),     // index
            receiver                    // array
        };
        
        value_t mapped_value = vm_call_function(vm, mapper, 3, callback_args);
        
        // If mapped value is an array, flatten it (add each element)
        if (mapped_value.type == VAL_ARRAY) {
            da_array mapped_array = mapped_value.as.array;
            size_t mapped_length = da_length(mapped_array);
            
            for (size_t j = 0; j < mapped_length; j++) {
                value_t* mapped_elem = (value_t*)da_get(mapped_array, j);
                if (mapped_elem) {
                    value_t retained = vm_retain(*mapped_elem);
                    da_push(result_array, &retained);
                }
            }
        } else {
            // If not an array, just add the value directly
            da_push(result_array, &mapped_value);
        }
    }
    
    return make_array(result_array);
}