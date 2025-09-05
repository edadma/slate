#include "adt_methods.h"
#include "vm.h"
#include "runtime_error.h"
#include "dynamic_string.h"
#include "dynamic_object.h"
#include "dynamic_array.h"
#include <stdio.h>
#include <string.h>
#include <stb_ds.h>

// ========================================
// ADT INSTANCE METHODS (for ADT instances like Some(42))
// ========================================

// ADT instance toString: "Some(42)"
value_t adt_instance_toString(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 1) {
        runtime_error(vm, "toString() method requires receiver");
        return make_null();
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "toString() can only be called on objects");
        return make_null();
    }
    
    // Get constructor information from the receiver's class
    if (!receiver.class || receiver.class->type != VAL_CLASS || !receiver.class->as.class) {
        return make_string("ADTInstance");
    }
    
    const char* constructor_name = receiver.class->as.class->name;
    if (!constructor_name) {
        return make_string("ADTInstance");
    }
    
    // Get parameter metadata from the constructor class
    da_array* param_names_array = NULL;
    if (receiver.class->as.class->static_properties) {
        param_names_array = (da_array*)do_get(receiver.class->as.class->static_properties, "__params__");
    }
    
    // Format the constructor display
    char buffer[512];
    
    if (!param_names_array || da_length(*param_names_array) == 0) {
        // No parameters - singleton case
        return make_string(constructor_name);
    } else {
        // Format with actual parameter names and values
        snprintf(buffer, sizeof(buffer), "%s(", constructor_name);
        
        for (int i = 0; i < (int)da_length(*param_names_array); i++) {
            ds_string* param_name_ptr = (ds_string*)da_get(*param_names_array, i);
            if (param_name_ptr) {
                value_t** param_value_ptr = (value_t**)do_get(receiver.as.object, *param_name_ptr);
                if (param_value_ptr && *param_value_ptr) {
                    value_t* param_value = *param_value_ptr;
                    if (i > 0) strcat(buffer, ", ");
                    
                    // Format the parameter value
                    if (param_value->type == VAL_INT32) {
                        char temp[32];
                        snprintf(temp, sizeof(temp), "%d", param_value->as.int32);
                        strcat(buffer, temp);
                    } else if (param_value->type == VAL_STRING) {
                        char temp[128];
                        snprintf(temp, sizeof(temp), "\"%s\"", param_value->as.string);
                        strcat(buffer, temp);
                    } else if (param_value->type == VAL_BOOLEAN) {
                        strcat(buffer, param_value->as.boolean ? "true" : "false");
                    } else if (param_value->type == VAL_NULL) {
                        strcat(buffer, "null");
                    } else {
                        strcat(buffer, "...");
                    }
                } else {
                    if (i > 0) strcat(buffer, ", ");
                    strcat(buffer, "?");
                }
            }
        }
        strcat(buffer, ")");
        
        return make_string(buffer);
    }
}

// ADT instance equals: structural equality
value_t adt_instance_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
        return make_boolean(0);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "equals() can only be called on objects");
        return make_boolean(0);
    }
    
    // Only ADT objects can be equal to ADT objects
    if (other.type != VAL_OBJECT) {
        return make_boolean(0);
    }
    
    // Identity equality check
    if (receiver.as.object == other.as.object) {
        return make_boolean(1);
    }
    
    // Get type information from both objects
    ds_string* type1 = (ds_string*)do_get(receiver.as.object, "__type");
    ds_string* case_type1 = (ds_string*)do_get(receiver.as.object, "__case_type");
    ds_string* type2 = (ds_string*)do_get(other.as.object, "__type");
    ds_string* case_type2 = (ds_string*)do_get(other.as.object, "__case_type");
    
    // Both must be ADT instances with same type and case
    if (!type1 || !case_type1 || !type2 || !case_type2) {
        return make_boolean(0);
    }
    
    if (strcmp(*type1, *type2) != 0 || strcmp(*case_type1, *case_type2) != 0) {
        return make_boolean(0);
    }
    
    // For singletons, type and case equality is sufficient
    if (strcmp(*case_type1, "singleton") == 0) {
        return make_boolean(1);
    }
    
    // For constructors, compare all parameters
    // TODO: Implement proper parameter comparison once we have parameter names
    // For now, fall back to generic object comparison
    const char** keys1 = do_get_own_keys(receiver.as.object);
    const char** keys2 = do_get_own_keys(other.as.object);
    
    size_t count1 = arrlen(keys1);
    size_t count2 = arrlen(keys2);
    
    if (count1 != count2) {
        arrfree(keys1);
        arrfree(keys2);
        return make_boolean(0);
    }
    
    // Compare all properties (including __type, __case_type and parameters)
    for (size_t i = 0; i < count1; i++) {
        value_t* val1 = (value_t*)do_get(receiver.as.object, keys1[i]);
        value_t* val2 = (value_t*)do_get(other.as.object, keys1[i]);
        
        if (!val1 || !val2) {
            arrfree(keys1);
            arrfree(keys2);
            return make_boolean(0);
        }
        
        // TODO: Use proper equals method dispatch here
        // For now, use simple value comparison
        if (val1->type != val2->type) {
            arrfree(keys1);
            arrfree(keys2);
            return make_boolean(0);
        }
    }
    
    arrfree(keys1);
    arrfree(keys2);
    return make_boolean(1);
}

// ADT instance hash: hash based on case name + parameters
value_t adt_instance_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 1) {
        runtime_error(vm, "hash() method requires receiver");
        return make_null();
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "hash() can only be called on objects");
        return make_null();
    }
    
    // Get ADT type information for hashing
    ds_string* type_name = (ds_string*)do_get(receiver.as.object, "__type");
    ds_string* case_type = (ds_string*)do_get(receiver.as.object, "__case_type");
    
    if (!type_name || !case_type) {
        return make_int32(0);
    }
    
    // Simple hash based on case name
    // TODO: Include parameter values in hash
    uint32_t hash = 2166136261u; // FNV offset basis
    const char* name = *type_name;
    while (*name) {
        hash ^= (uint32_t)*name++;
        hash *= 16777619u; // FNV prime
    }
    
    return make_int32((int32_t)hash);
}

// ========================================
// ADT CLASS STATIC METHODS (for constructor classes like Some, None)
// ========================================

// ADT class toString: "Some"
value_t adt_class_toString(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 1) {
        runtime_error(vm, "toString() method requires receiver");
        return make_null();
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_CLASS) {
        runtime_error(vm, "toString() can only be called on classes");
        return make_null();
    }
    
    // Return the class name
    if (receiver.as.class && receiver.as.class->name) {
        return make_string(receiver.as.class->name);
    }
    
    return make_string("ADTClass");
}

// ADT class equals: constructor identity
value_t adt_class_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
        return make_boolean(0);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_CLASS) {
        runtime_error(vm, "equals() can only be called on classes");
        return make_boolean(0);
    }
    
    // Only classes can be equal to classes
    if (other.type != VAL_CLASS) {
        return make_boolean(0);
    }
    
    // Compare class identity
    return make_boolean(receiver.as.class == other.as.class);
}

// ADT class hash: hash based on class name
value_t adt_class_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 1) {
        runtime_error(vm, "hash() method requires receiver");
        return make_null();
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_CLASS) {
        runtime_error(vm, "hash() can only be called on classes");
        return make_null();
    }
    
    // Hash based on class name
    if (!receiver.as.class || !receiver.as.class->name) {
        return make_int32(0);
    }
    
    uint32_t hash = 2166136261u; // FNV offset basis
    const char* name = receiver.as.class->name;
    while (*name) {
        hash ^= (uint32_t)*name++;
        hash *= 16777619u; // FNV prime
    }
    
    return make_int32((int32_t)hash);
}