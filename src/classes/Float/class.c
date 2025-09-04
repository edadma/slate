#include "float.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "number.h"

// Global Float class storage
value_t* global_float_class = NULL;

// Initialize the Float class
void float_class_init(vm_t* vm) {
    // Get the Number class to inherit from
    value_t* number_class_ptr = (value_t*)do_get(vm->globals, "Number");
    if (!number_class_ptr || number_class_ptr->type != VAL_CLASS) {
        runtime_error(vm, "Cannot initialize Float class: Number class not found");
        return;
    }
    
    // Create the Float class with its prototype
    do_object float_proto = do_create(NULL);
    
    // Add Float-specific methods to Float prototype
    value_t float_hash_method = make_native(builtin_float_hash);
    do_set(float_proto, "hash", &float_hash_method, sizeof(value_t));
    
    value_t float_equals_method = make_native(builtin_float_equals);
    do_set(float_proto, "equals", &float_equals_method, sizeof(value_t));
    
    value_t float_to_string_method = make_native(builtin_float_to_string);
    do_set(float_proto, "toString", &float_to_string_method, sizeof(value_t));
    
    value_t float_abs_method = make_native(builtin_float_abs);
    do_set(float_proto, "abs", &float_abs_method, sizeof(value_t));
    
    value_t float_sign_method = make_native(builtin_float_sign);
    do_set(float_proto, "sign", &float_sign_method, sizeof(value_t));
    
    value_t float_is_finite_method = make_native(builtin_float_is_finite);
    do_set(float_proto, "isFinite", &float_is_finite_method, sizeof(value_t));
    
    value_t float_is_integer_method = make_native(builtin_float_is_integer);
    do_set(float_proto, "isInteger", &float_is_integer_method, sizeof(value_t));
    
    value_t float_sqrt_method = make_native(builtin_float_sqrt);
    do_set(float_proto, "sqrt", &float_sqrt_method, sizeof(value_t));
    
    value_t float_floor_method = make_native(builtin_float_floor);
    do_set(float_proto, "floor", &float_floor_method, sizeof(value_t));
    
    value_t float_ceil_method = make_native(builtin_float_ceil);
    do_set(float_proto, "ceil", &float_ceil_method, sizeof(value_t));
    
    value_t float_round_method = make_native(builtin_float_round);
    do_set(float_proto, "round", &float_round_method, sizeof(value_t));
    
    value_t float_sin_method = make_native(builtin_float_sin);
    do_set(float_proto, "sin", &float_sin_method, sizeof(value_t));
    
    value_t float_cos_method = make_native(builtin_float_cos);
    do_set(float_proto, "cos", &float_cos_method, sizeof(value_t));
    
    value_t float_tan_method = make_native(builtin_float_tan);
    do_set(float_proto, "tan", &float_tan_method, sizeof(value_t));
    
    value_t float_exp_method = make_native(builtin_float_exp);
    do_set(float_proto, "exp", &float_exp_method, sizeof(value_t));
    
    value_t float_ln_method = make_native(builtin_float_ln);
    do_set(float_proto, "ln", &float_ln_method, sizeof(value_t));
    
    value_t float_asin_method = make_native(builtin_float_asin);
    do_set(float_proto, "asin", &float_asin_method, sizeof(value_t));
    
    value_t float_acos_method = make_native(builtin_float_acos);
    do_set(float_proto, "acos", &float_acos_method, sizeof(value_t));
    
    value_t float_atan_method = make_native(builtin_float_atan);
    do_set(float_proto, "atan", &float_atan_method, sizeof(value_t));
    
    value_t float_degrees_method = make_native(builtin_float_degrees);
    do_set(float_proto, "degrees", &float_degrees_method, sizeof(value_t));
    
    value_t float_radians_method = make_native(builtin_float_radians);
    do_set(float_proto, "radians", &float_radians_method, sizeof(value_t));
    
    // Create the Float class
    value_t float_class = make_class("Float", float_proto);
    
    // Set the factory function to allow Float(value)
    float_class.as.class->factory = float_factory;
    
    // Set Number as parent class for inheritance
    float_class.class = number_class_ptr;
    
    // Store in globals
    do_set(vm->globals, "Float", &float_class, sizeof(value_t));
    
    // Store a global reference for use in make_float functions
    static value_t float_class_storage;
    float_class_storage = vm_retain(float_class);
    global_float_class = &float_class_storage;
}