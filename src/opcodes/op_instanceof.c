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
        class_t* value_class = value_val.class->as.class;
        
        // Simple class comparison - could be extended for inheritance
        is_instance = (value_class == target_class);
    }
    
    vm_push(vm, make_boolean(is_instance));
    vm_release(value_val);
    vm_release(class_val);
    
    return VM_OK;
}