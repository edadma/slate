#include "vm.h"
#include "runtime_error.h"
#include <stdlib.h>

vm_result op_build_array(slate_vm* vm) {
    uint16_t element_count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Create new dynamic array for elements
    da_array array = da_new(sizeof(value_t));
    
    // Collect all elements from stack (they're in reverse order)
    value_t* elements = malloc(sizeof(value_t) * element_count);
    for (int i = element_count - 1; i >= 0; i--) {
        elements[i] = vm_pop(vm);
        // Check if trying to store undefined (not a first-class value)
        if (elements[i].type == VAL_UNDEFINED) {
            slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Cannot store 'undefined' in array - it is not a value");
            free(elements);
            da_release(&array);
        }
    }
    
    // Add elements to array in correct order
    for (uint16_t i = 0; i < element_count; i++) {
        da_push(array, &elements[i]);
    }
    
    free(elements);
    value_t result = make_array(array);
    vm_push(vm, result);
    return VM_OK;
}