#include "vm.h"

vm_result op_in(vm_t* vm) {
    value_t object = vm_pop(vm);
    value_t property = vm_pop(vm);

    // Property must be a string for property lookup
    if (property.type != VAL_STRING) {
        vm_runtime_error_with_values(vm, "Property name must be a string, got %s", &property, NULL, property.debug);
        vm_release(object);
        vm_release(property);
        return VM_RUNTIME_ERROR;
    }

    const char* prop_name = property.as.string;
    bool found = false;

    // Check different object types
    if (object.type == VAL_OBJECT) {
        // Check own properties
        value_t* prop_value = (value_t*)do_get(object.as.object, prop_name);
        found = (prop_value != NULL);
    } else if (object.type == VAL_ARRAY) {
        // For arrays, check if property name is a valid numeric index
        // or if it's a built-in array method/property
        if (strcmp(prop_name, "length") == 0) {
            found = true;
        } else {
            // Check if it's a numeric index within bounds
            char* endptr;
            long index = strtol(prop_name, &endptr, 10);
            if (*endptr == '\0' && index >= 0) {
                size_t array_len = da_length(object.as.array);
                found = (index < (long)array_len);
            }
        }
    } else if (object.type == VAL_STRING) {
        // For strings, check built-in string properties/methods
        if (strcmp(prop_name, "length") == 0) {
            found = true;
        } else {
            // Check if it's a numeric index within bounds
            char* endptr;
            long index = strtol(prop_name, &endptr, 10);
            if (*endptr == '\0' && index >= 0) {
                size_t str_len = strlen(object.as.string);
                found = (index < (long)str_len);
            }
        }
    }
    
    // Also check the prototype chain via class if object has one
    if (!found && object.class && object.class->type == VAL_CLASS) {
        class_t* cls = object.class->as.class;
        value_t* prop_value = lookup_instance_property(cls, prop_name);
        found = (prop_value != NULL);
    }

    vm_push(vm, make_boolean_with_debug(found, property.debug));

    vm_release(object);
    vm_release(property);
    return VM_OK;
}