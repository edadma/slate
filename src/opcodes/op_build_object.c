#include "vm.h"
#include "runtime_error.h"

vm_result op_build_object(vm_t* vm) {
    uint16_t pair_count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Create new dynamic object
    do_object object = do_create(NULL); // NULL release function for now
    if (!object) {
        runtime_error(vm, "Failed to create object");
        return VM_RUNTIME_ERROR;
    }

    // Pop key-value pairs from stack (they're in reverse order)
    for (int i = 0; i < pair_count; i++) {
        value_t value = vm_pop(vm);
        value_t key = vm_pop(vm);

        // Check if trying to store undefined (not a first-class value)
        if (value.type == VAL_UNDEFINED) {
            do_release(&object);
            vm_release(key);
            vm_release(value);
            runtime_error(vm, "Cannot store 'undefined' in object - it is not a value");
            return VM_RUNTIME_ERROR;
        }

        // Key must be a string
        if (key.type != VAL_STRING) {
            do_release(&object);
            vm_release(key);
            vm_release(value);
            runtime_error(vm, "Object key must be a string");
            return VM_RUNTIME_ERROR;
        }

        // Set property in object
        if (do_set(object, key.as.string, &value, sizeof(value_t)) != 0) {
            do_release(&object);
            vm_release(key);
            vm_release(value);
            runtime_error(vm, "Failed to set object property");
            return VM_RUNTIME_ERROR;
        }

        // Clean up key (value is now owned by the object)
        vm_release(key);
    }

    vm_push(vm, make_object(object));
    return VM_OK;
}