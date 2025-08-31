#include "vm.h"

vm_result op_build_object(slate_vm* vm) {
    uint16_t pair_count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Create new dynamic object
    do_object object = do_create(NULL); // NULL release function for now
    if (!object) {
        vm_runtime_error_with_debug(vm, "Failed to create object");
        return VM_RUNTIME_ERROR;
    }

    // Pop key-value pairs from stack (they're in reverse order)
    for (int i = 0; i < pair_count; i++) {
        value_t value = vm_pop(vm);
        value_t key = vm_pop(vm);

        // Check if trying to store undefined (not a first-class value)
        if (value.type == VAL_UNDEFINED) {
            vm_runtime_error_with_debug(vm, "Cannot store 'undefined' in object - it is not a value");
            do_release(&object);
            vm_release(key);
            vm_release(value);
            return VM_RUNTIME_ERROR;
        }

        // Key must be a string
        if (key.type != VAL_STRING) {
            vm_runtime_error_with_debug(vm, "Object key must be a string");
            do_release(&object);
            vm_release(key);
            vm_release(value);
            return VM_RUNTIME_ERROR;
        }

        // Set property in object
        if (do_set(object, key.as.string, &value, sizeof(value_t)) != 0) {
            vm_runtime_error_with_debug(vm, "Failed to set object property");
            do_release(&object);
            vm_release(key);
            vm_release(value);
            return VM_RUNTIME_ERROR;
        }

        // Clean up key (value is now owned by the object)
        vm_release(key);
    }

    vm_push(vm, make_object(object));
    return VM_OK;
}