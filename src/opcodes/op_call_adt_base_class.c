#include "vm.h"
#include "runtime_error.h"

vm_result op_call_adt_base_class(vm_t* vm) {
    // Stack: [name, instance_props, static_props]
    // Create base class for algebraic data type
    
    value_t static_props = vm_pop(vm);
    value_t instance_props = vm_pop(vm);  
    value_t name_val = vm_pop(vm);
    
    if (name_val.type != VAL_STRING) {
        vm_release(static_props);
        vm_release(instance_props);
        vm_release(name_val);
        runtime_error(vm, "ADT base class name must be a string");
        return VM_RUNTIME_ERROR;
    }
    
    // Create the base class
    do_object instance_properties = NULL;
    do_object static_properties = NULL;
    
    // Use provided properties or create empty ones
    if (instance_props.type == VAL_OBJECT) {
        instance_properties = instance_props.as.object;
        do_retain(instance_properties);
    }
    
    if (static_props.type == VAL_OBJECT) {
        static_properties = static_props.as.object;
        do_retain(static_properties);
    }
    
    value_t base_class = make_class_with_debug(name_val.as.string, instance_properties, static_properties, vm->current_debug);
    
    // ADT base classes don't have factory functions (they're not directly instantiable)
    base_class.as.class->factory = NULL;
    
    vm_push(vm, base_class);
    
    // Clean up
    vm_release(static_props);
    vm_release(instance_props);  
    vm_release(name_val);
    
    return VM_OK;
}