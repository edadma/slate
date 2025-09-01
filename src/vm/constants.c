#include "vm.h"
#include <assert.h>

// Constant pool management
size_t vm_add_constant(slate_vm* vm, value_t value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(slate_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
}

// Function table management
size_t vm_add_function(slate_vm* vm, function_t* function) {
    size_t index = vm->functions->length;
    DA_PUSH(vm->functions, function);
    return index;
}

function_t* vm_get_function(slate_vm* vm, size_t index) {
    assert(index < vm->functions->length);
    return DA_AT(vm->functions, index, function_t*);
}

// Note: Object operations now use dynamic_object.h
// No separate functions needed

// Note: Array operations now use dynamic_array.h
// No separate functions needed