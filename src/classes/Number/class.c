#include "number.h"
#include "builtins.h"
#include "dynamic_object.h"

// Global Number class storage
value_t* global_number_class = NULL;

// Create abstract Number superclass (no instance methods - purely for instanceof)
void number_class_init(vm_t* vm) {
    // Get the Value class to inherit from
    value_t* value_class_ptr = (value_t*)do_get(vm->globals, "Value");
    if (!value_class_ptr || value_class_ptr->type != VAL_CLASS) {
        runtime_error(vm, "Cannot initialize Number class: Value class not found");
        return;
    }
    
    // Create Number class prototype with methods that work across all numeric types
    do_object number_proto = do_create(NULL);
    
    // Add methods that work for all numeric types
    value_t min_method = make_native(builtin_number_min);
    do_set(number_proto, "min", &min_method, sizeof(value_t));
    
    value_t max_method = make_native(builtin_number_max);
    do_set(number_proto, "max", &max_method, sizeof(value_t));
    
    value_t equals_method = make_native(builtin_number_equals);
    do_set(number_proto, "equals", &equals_method, sizeof(value_t));
    
    // Create the Number class
    value_t number_class = make_class("Number", number_proto);
    
    // Number class has no factory - it's abstract
    number_class.as.class->factory = NULL;
    
    // Set Value as parent class for inheritance
    number_class.class = value_class_ptr;
    
    // Store as global Number class
    do_set(vm->globals, "Number", &number_class, sizeof(value_t));
}