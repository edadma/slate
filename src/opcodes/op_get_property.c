#include "vm.h"
#include "runtime_error.h"

vm_result op_get_property(vm_t* vm) {
    value_t property = vm_pop(vm);
    value_t object = vm_pop(vm);

    if (property.type != VAL_STRING) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Property name must be a string");
    }

    const char* prop_name = property.as.string;

    // For classes, check static properties first (e.g., Buffer.fromHex)
    if (object.type == VAL_CLASS) {
        class_t* cls = object.as.class;
        if (cls && cls->properties) {
            value_t* prop_value = (value_t*)do_get(cls->properties, prop_name);
            if (prop_value) {
                vm_push(vm, *prop_value);
                vm_release(object);
                vm_release(property);
                return VM_OK;
            }
        }
        // If not found in class properties, push undefined
        vm_push(vm, make_undefined());
        vm_release(object);
        vm_release(property);
        return VM_OK;
    }

    // For objects, check own properties first
    if (object.type == VAL_OBJECT) {
        value_t* prop_value = (value_t*)do_get(object.as.object, prop_name);
        if (prop_value) {
            vm_push(vm, *prop_value);
            vm_release(object);
            vm_release(property);
            return VM_OK;
        }
    }

    // Check the prototype chain via class - walk up inheritance hierarchy
    value_t* current_class = object.class;
    bool property_found = false;
    
    
    while (current_class && current_class->type == VAL_CLASS && !property_found) {
        // Get the class's prototype properties
        class_t* cls = current_class->as.class;
        if (cls && cls->properties) {
            value_t* prop_value = (value_t*)do_get(cls->properties, prop_name);
            if (prop_value) {
                // If it's a native function, create a bound method
                if (prop_value->type == VAL_NATIVE) {
                    vm_push(vm, make_bound_method(object, prop_value->as.native));
                    property_found = true;
                } else {
                    vm_push(vm, *prop_value);
                    property_found = true;
                }
                break;
            }
        }
        // Move to parent class if any
        // For now, no inheritance - just break
        break;
    }
    
    if (!property_found) {
        // Push undefined for non-existent properties (like JavaScript)
        vm_push(vm, make_undefined());
    }
    
    // Clean up operands
    vm_release(object);
    vm_release(property);
    return VM_OK;
}