#include "vm.h"
#include "runtime_error.h"

// Constant pool management
size_t vm_add_constant(slate_vm* vm, value_t value) {
    if (!(vm->constant_count < vm->constant_capacity)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Constant pool overflow: cannot add more constants");
    }
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(slate_vm* vm, size_t index) {
    if (!(index < vm->constant_count)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Invalid constant index: %zu (max: %zu)", index, vm->constant_count - 1);
    }
    return vm->constants[index];
}

// Function table management
size_t vm_add_function(slate_vm* vm, function_t* function) {
    size_t index = vm->functions->length;
    DA_PUSH(vm->functions, function);
    return index;
}

function_t* vm_get_function(slate_vm* vm, size_t index) {
    if (!(index < vm->functions->length)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Invalid function index: %zu (max: %zu)", index, vm->functions->length - 1);
    }
    return DA_AT(vm->functions, index, function_t*);
}

// Note: Object operations now use dynamic_object.h
// No separate functions needed

// Note: Array operations now use dynamic_array.h
// No separate functions needed