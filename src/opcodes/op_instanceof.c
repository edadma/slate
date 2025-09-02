#include "vm.h"

vm_result op_instanceof(vm_t* vm) {
    value_t class_val = vm_pop(vm);  // The class to check against
    value_t value_val = vm_pop(vm);  // The value to check
    
    bool is_instance = false;
    
    // Check if the class_val is actually a class
    if (class_val.type != VAL_CLASS) {
        // instanceof requires a class on the right side, not a primitive type name
        vm_release(value_val);
        vm_release(class_val);
        return VM_RUNTIME_ERROR;
    }
    
    class_t* target_class = class_val.as.class;
    
    // Check if the value has a class and if it matches
    if (value_val.class && value_val.class->type == VAL_CLASS) {
        // Check inheritance hierarchy by walking up the class chain
        value_t* current_class_val = value_val.class;
        while (current_class_val && current_class_val->type == VAL_CLASS) {
            if (current_class_val->as.class == target_class) {
                is_instance = true;
                break;
            }
            // Move up the inheritance chain
            current_class_val = current_class_val->class;
        }
    }
    
    vm_push(vm, make_boolean(is_instance));
    vm_release(value_val);
    vm_release(class_val);
    
    return VM_OK;
}