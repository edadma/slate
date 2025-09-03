#include "vm.h"
#include "runtime_error.h"

vm_result op_set_property(vm_t* vm) {
    // Stack order: object, property_name, value (top)
    // Pop the value to assign
    value_t value = vm_pop(vm);
    
    // Pop the property name
    value_t property_name = vm_pop(vm);
    
    // Pop the object
    value_t object = vm_pop(vm);
    
    // Validate that we have an object
    if (object.type != VAL_OBJECT) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Can only set properties on objects");
        vm_release(value);
        vm_release(property_name);
        vm_release(object);
        return VM_RUNTIME_ERROR;
    }
    
    // Validate that the property name is a string
    if (property_name.type != VAL_STRING) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Property name must be a string");
        vm_release(value);
        vm_release(property_name);
        vm_release(object);
        return VM_RUNTIME_ERROR;
    }
    
    // Set the property in the object
    // First check if the property already exists and release the old value
    value_t* existing_value = (value_t*)do_get(object.as.object, property_name.as.string);
    if (existing_value) {
        vm_release(*existing_value);
    }
    
    // Set the new property value (retain it since it's now stored in the object)
    value_t new_value = vm_retain(value);
    do_set(object.as.object, property_name.as.string, &new_value, sizeof(value_t));
    
    // Push the assigned value back onto the stack (for assignment expressions)
    vm_push(vm, vm_retain(value));
    
    // Clean up
    vm_release(value);
    vm_release(property_name);
    vm_release(object);
    
    return VM_OK;
}