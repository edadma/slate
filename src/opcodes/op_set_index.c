#include "vm.h"
#include "runtime_error.h"

vm_result op_set_index(vm_t* vm) {
    // Stack order: array, index, value (top)
    // Pop the value to assign
    value_t value = vm_pop(vm);
    
    // Pop the index
    value_t index_val = vm_pop(vm);
    
    // Pop the array
    value_t array_val = vm_pop(vm);
    
    // Validate that we have an array
    if (array_val.type != VAL_ARRAY) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Can only set index on arrays");
        vm_release(value);
        vm_release(index_val);
        vm_release(array_val);
        return VM_RUNTIME_ERROR;
    }
    
    // Validate that the index is an integer
    if (index_val.type != VAL_INT32) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Array index must be an integer");
        vm_release(value);
        vm_release(index_val);
        vm_release(array_val);
        return VM_RUNTIME_ERROR;
    }
    
    int32_t index = index_val.as.int32;
    size_t array_length = da_length(array_val.as.array);
    
    // Check bounds
    if (index < 0 || index >= array_length) {
        slate_runtime_error(vm, ERR_RANGE, __FILE__, __LINE__, -1, "Array index out of bounds: %d (array length: %zu)", index, array_length);
        vm_release(value);
        vm_release(index_val);
        vm_release(array_val);
        return VM_RUNTIME_ERROR;
    }
    
    // Get the current element and release it
    value_t* current_element = (value_t*)da_get(array_val.as.array, index);
    vm_release(*current_element);
    
    // Set the new value (retain it since it's now stored in the array)
    value_t new_value = vm_retain(value);
    da_set(array_val.as.array, index, &new_value);
    
    // Push the assigned value back onto the stack (for assignment expressions)
    vm_push(vm, vm_retain(value));
    
    // Clean up
    vm_release(value);
    vm_release(index_val);
    vm_release(array_val);
    
    return VM_OK;
}