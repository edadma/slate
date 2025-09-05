#include "adt_methods.h"
#include "vm.h"
#include "runtime_error.h"
#include "dynamic_string.h"
#include "dynamic_object.h"
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
    
    // Get ADT type information
    ds_string* case_type = (ds_string*)do_get(receiver.as.object, "__case_type");
    ds_string* type_name = (ds_string*)do_get(receiver.as.object, "__type");
    
    const char* constructor_name = "ADTInstance"; // fallback
    
    // Get the constructor name from the stored type name
    if (type_name) {
        constructor_name = *type_name;
    }
    
    if (!case_type) {
        return make_string(constructor_name);
    }
    
    if (strcmp(*case_type, "singleton") == 0) {
        // Singleton case: just return the case name
        return make_string(constructor_name);
    } else if (strcmp(*case_type, "constructor") == 0) {
        // Constructor case: show case name with actual parameters
        char buffer[256];
        
        // Count how many parameters we have
        int param_count = 0;
        const char** keys = do_get_own_keys(receiver.as.object);
        if (keys) {
            for (int i = 0; i < (int)arrlen(keys); i++) {
                if (strncmp(keys[i], "param_", 6) == 0) {
                    param_count++;
                }
            }
            arrfree(keys);
        }
        
        if (param_count == 0) {
            // No parameters - just show constructor name
            snprintf(buffer, sizeof(buffer), "%s", constructor_name);
        } else if (param_count == 1) {
            // Single parameter - show the actual value
            value_t* param = (value_t*)do_get(receiver.as.object, "param_0");
            if (param) {
                // Show the constructor with the single parameter value
                if (param->type == VAL_INT32) {
                    snprintf(buffer, sizeof(buffer), "%s(%d)", constructor_name, param->as.int32);
                } else if (param->type == VAL_STRING) {
                    snprintf(buffer, sizeof(buffer), "%s(\"%s\")", constructor_name, param->as.string);
                } else {
                    snprintf(buffer, sizeof(buffer), "%s(...)", constructor_name);
                }
            } else {
                snprintf(buffer, sizeof(buffer), "%s(...)", constructor_name);
            }
        } else {
            // Multiple parameters - show constructor with ellipsis for now
            snprintf(buffer, sizeof(buffer), "%s(...)", constructor_name);
        }
        
        return make_string(buffer);
    }
    
    return make_string("ADTInstance");
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