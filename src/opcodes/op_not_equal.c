#include "vm.h"
#include "runtime_error.h"

vm_result op_not_equal(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);
    
    // Call .equals() method on the left operand using method dispatch
    value_t* current_class = a.class;
    
    while (current_class && current_class->type == VAL_CLASS) {
        class_t* cls = current_class->as.class;
        if (cls && cls->properties) {
            value_t* equals_method = (value_t*)do_get(cls->properties, "equals");
            if (equals_method && equals_method->type == VAL_NATIVE) {
                // Call the .equals() method with both operands as arguments
                value_t args[2] = { a, b };
                native_t native_func = (native_t)equals_method->as.native;
                value_t equals_result = native_func(vm, 2, args);
                
                // Negate the result for not-equal
                if (equals_result.type == VAL_BOOLEAN) {
                    vm_push(vm, make_boolean(!equals_result.as.boolean));
                } else {
                    vm_push(vm, make_boolean(true)); // If equals fails, assume not equal
                }
                
                vm_release(a);
                vm_release(b);
                return VM_OK;
            }
        }
        // Move to parent class if any
        current_class = current_class->class;
    }
    
    // If no .equals() method found, runtime error - all classes must have equals
    vm_release(a);
    vm_release(b);
    runtime_error(vm, "Type %s has no .equals() method", value_type_name(a.type));
}