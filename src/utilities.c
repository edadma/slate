#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "builtins.h"
#include "date.h"
#include "datetime.h"
#include "instant.h"
#include "timezone.h"
#include "vm.h"

// External reference to global VM pointer (defined in vm/lifecycle.c)
extern vm_t* g_current_vm;

// Value utility functions
int is_falsy(value_t value) {
    switch (value.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return !value.as.boolean;
    case VAL_INT32:
        return value.as.int32 == 0;
    case VAL_BIGINT:
        return di_is_zero(value.as.bigint);
    case VAL_FLOAT32:
        return value.as.float32 == 0.0f;
    case VAL_FLOAT64:
        return value.as.float64 == 0.0;
    case VAL_STRING:
        return value.as.string == NULL || strlen(value.as.string) == 0;
    case VAL_BUFFER:
        return value.as.buffer == NULL || db_size(value.as.buffer) == 0;
    case VAL_BUFFER_BUILDER:
        return value.as.builder == NULL;
    case VAL_BUFFER_READER:
        return value.as.reader == NULL;
    default:
        return 0;
    }
}

int is_truthy(value_t value) { return !is_falsy(value); }

int is_number(value_t value) {
    return value.type == VAL_INT32 || value.type == VAL_BIGINT || value.type == VAL_FLOAT32 ||
        value.type == VAL_FLOAT64;
}

// Generalized numeric comparison function
// Returns: -1 if a < b, 0 if a == b, 1 if a > b
int compare_numbers(value_t a, value_t b) {
    // Handle same-type comparisons first (most efficient)
    if (a.type == b.type) {
        switch (a.type) {
        case VAL_INT32:
            if (a.as.int32 < b.as.int32)
                return -1;
            if (a.as.int32 > b.as.int32)
                return 1;
            return 0;

        case VAL_BIGINT:
            if (di_lt(a.as.bigint, b.as.bigint))
                return -1;
            if (di_gt(a.as.bigint, b.as.bigint))
                return 1;
            return 0;

        case VAL_FLOAT32: {
            float fa = a.as.float32;
            float fb = b.as.float32;
            if (fa < fb)
                return -1;
            if (fa > fb)
                return 1;
            return 0;
        }

        case VAL_FLOAT64: {
            double da = a.as.float64;
            double db = b.as.float64;
            if (da < db)
                return -1;
            if (da > db)
                return 1;
            return 0;
        }

        default:
            return 0; // Should not happen for numeric types
        }
    }

    // Handle cross-type comparisons
    // Priority order: Check for any floating point first, then handle integer comparisons

    // If either is floating point, convert both to appropriate floating point type
    if (a.type == VAL_FLOAT64 || b.type == VAL_FLOAT64 || a.type == VAL_FLOAT32 || b.type == VAL_FLOAT32) {

        // Use double for highest precision
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
            : (a.type == VAL_FLOAT32)        ? (double)a.as.float32
                                             : a.as.float64;

        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
            : (b.type == VAL_FLOAT32)        ? (double)b.as.float32
                                             : b.as.float64;

        if (a_val < b_val)
            return -1;
        if (a_val > b_val)
            return 1;
        return 0;
    }

    // Both are integers (INT32 and/or BIGINT) - handle without floating point
    if ((a.type == VAL_INT32 || a.type == VAL_BIGINT) && (b.type == VAL_INT32 || b.type == VAL_BIGINT)) {

        // Convert both to BigInt for accurate comparison
        di_int a_big, b_big;
        int a_needs_release = 0, b_needs_release = 0;

        if (a.type == VAL_BIGINT) {
            a_big = a.as.bigint;
        } else {
            a_big = di_from_int32(a.as.int32);
            a_needs_release = 1;
        }

        if (b.type == VAL_BIGINT) {
            b_big = b.as.bigint;
        } else {
            b_big = di_from_int32(b.as.int32);
            b_needs_release = 1;
        }

        int result;
        if (di_lt(a_big, b_big)) {
            result = -1;
        } else if (di_gt(a_big, b_big)) {
            result = 1;
        } else {
            result = 0;
        }

        // Clean up temporary BigInts
        if (a_needs_release)
            di_release(&a_big);
        if (b_needs_release)
            di_release(&b_big);

        return result;
    }

    // Should not reach here for valid numeric types
    return 0;
}

// Helper function to call .toString() method on values using method dispatch
value_t call_toString_method(vm_t* vm, value_t value) {
    // For classes (VAL_CLASS), check static properties first
    if (value.type == VAL_CLASS && value.as.class) {
        value_t* static_toString = lookup_static_property(value.as.class, "toString");
        if (static_toString && static_toString->type == VAL_NATIVE) {
            value_t args[1] = { value };
            native_t native_func = (native_t)static_toString->as.native;
            value_t result = native_func(vm, 1, args);
            
            if (result.type == VAL_STRING) {
                return result;
            }
        }
    }
    
    // For objects, first check instance properties for toString method
    if (value.type == VAL_OBJECT && value.as.object) {
        value_t* instance_toString = (value_t*)do_get(value.as.object, "toString");
        if (instance_toString && instance_toString->type == VAL_NATIVE) {
            value_t args[1] = { value };
            native_t native_func = (native_t)instance_toString->as.native;
            value_t result = native_func(vm, 1, args);
            
            if (result.type == VAL_STRING) {
                return result;
            }
        }
    }
    
    // Then check class prototype chain for instance methods
    value_t* current_class = value.class;
    
    while (current_class && current_class->type == VAL_CLASS) {
        class_t* cls = current_class->as.class;
        value_t* toString_method = lookup_instance_property(cls, "toString");
        if (toString_method && toString_method->type == VAL_NATIVE) {
            value_t args[1] = { value };
            native_t native_func = (native_t)toString_method->as.native;
            value_t result = native_func(vm, 1, args);
            
            if (result.type == VAL_STRING) {
                return result;
            }
            break; // Method found but didn't return string - stop looking
        }
        // Move to parent class if any
        current_class = current_class->class;
    }
    
    // No toString method found or it didn't return a string - return null
    return make_null();
}

// Helper function to convert values to display string (with quotes for strings, for use inside aggregates)
ds_string display_value_to_string(vm_t* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING: {
        // Add quotes around strings for display inside aggregates
        ds_string quoted = ds_new("\"");
        if (value.as.string) {
            ds_string temp1 = ds_concat(quoted, value.as.string);
            ds_release(&quoted);
            quoted = temp1;
        }
        ds_string end_quote = ds_new("\"");
        ds_string temp2 = ds_concat(quoted, end_quote);
        ds_release(&quoted);
        ds_release(&end_quote);
        return temp2;
    }
    default:
        // For all other types, use the toString method
        return call_toString_for_string_conversion(vm, value);
    }
}

// Helper function to call .equals() method on values using method dispatch
int call_equals_method(vm_t* vm, value_t a, value_t b) {
    // For classes (VAL_CLASS), check static properties first
    if (a.type == VAL_CLASS && a.as.class) {
        value_t* static_equals = lookup_static_property(a.as.class, "equals");
        if (static_equals && static_equals->type == VAL_NATIVE) {
            value_t args[2] = { a, b };
            native_t native_func = (native_t)static_equals->as.native;
            value_t result = native_func(vm, 2, args);
            
            if (result.type == VAL_BOOLEAN) {
                return result.as.boolean;
            }
            return 0; // Default to false if method doesn't return boolean
        }
    }
    
    // Call .equals() method on the left operand using instance method dispatch
    value_t* current_class = a.class;
    
    while (current_class && current_class->type == VAL_CLASS) {
        class_t* cls = current_class->as.class;
        value_t* equals_method = lookup_instance_property(cls, "equals");
        if (equals_method && equals_method->type == VAL_NATIVE) {
            value_t args[2] = { a, b };
            native_t native_func = (native_t)equals_method->as.native;
            value_t result = native_func(vm, 2, args);
            
            if (result.type == VAL_BOOLEAN) {
                return result.as.boolean;
            }
            return 0; // Default to false if method doesn't return boolean
        }
        // Move to parent class if any
        current_class = current_class->class;
    }
    
    // No equals method found - default to false
    return 0;
}

void print_value(vm_t* vm, value_t value) {
    // Strings get special treatment - we want to show them with quotes in print_value
    // This is different from print_for_builtin which shows strings without quotes
    if (value.type == VAL_STRING) {
        printf("\"%s\"", value.as.string ? value.as.string : "");
        return;
    }
    
    // For all other types, use the toString method system
    ds_string str = call_toString_for_string_conversion(vm, value);
    printf("%s", str);
    ds_release(&str);
}

float value_to_float32(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return (float)value.as.int32;
    case VAL_BIGINT:
        return (float)di_to_double(value.as.bigint);
    case VAL_FLOAT32:
        return value.as.float32;
    case VAL_FLOAT64:
        return (float)value.as.float64;
    default:
        runtime_error(g_current_vm, "Cannot convert %s to number", value_type_name(value.type));
        return 0.0f; // Never reached, but keeps compiler happy
    }
}

double value_to_float64(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return (double)value.as.int32;
    case VAL_BIGINT:
        return di_to_double(value.as.bigint);
    case VAL_FLOAT32:
        return (double)value.as.float32;
    case VAL_FLOAT64:
        return value.as.float64;
    default:
        runtime_error(g_current_vm, "Cannot convert %s to number", value_type_name(value.type));
        return 0.0; // Never reached, but keeps compiler happy
    }
}

bool is_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return true;
    case VAL_BIGINT:
        // Check if BigInt fits in int32 range
        int32_t dummy;
        return di_to_int32(value.as.bigint, &dummy);
    case VAL_FLOAT64:
        // Check if the number is a whole number that fits in int range
        return value.as.float64 == floor(value.as.float64) && value.as.float64 >= INT_MIN &&
            value.as.float64 <= INT_MAX;
    default:
        return false;
    }
}

int value_to_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return value.as.int32;
    case VAL_BIGINT:
        // Convert BigInt to int, checking for overflow
        int32_t result;
        if (di_to_int32(value.as.bigint, &result)) {
            return result;
        } else {
            char* str = di_to_string(value.as.bigint, 10);
            runtime_error(g_current_vm, "BigInt value %s too large for integer", str);
            free(str);
            return 0; // Never reached
        }
    case VAL_FLOAT64:
        // Check if it's a valid integer
        if (value.as.float64 == floor(value.as.float64) && value.as.float64 >= INT_MIN && value.as.float64 <= INT_MAX) {
            return (int)value.as.float64;
        } else {
            runtime_error(g_current_vm, "Number %g is not a valid integer", value.as.float64);
            return 0; // Never reached
        }
    default:
        runtime_error(g_current_vm, "Cannot convert %s to integer", value_type_name(value.type));
        return 0; // Never reached, but keeps compiler happy
    }
}

// Property lookup functions

// Lookup a static property (method called on class object)
value_t* lookup_static_property(class_t* cls, const char* prop_name) {
    if (!cls || !cls->static_properties || !prop_name) {
        return NULL;
    }
    return (value_t*)do_get(cls->static_properties, prop_name);
}

// Lookup an instance property (method called on class instance)
value_t* lookup_instance_property(class_t* cls, const char* prop_name) {
    if (!cls || !cls->instance_properties || !prop_name) {
        return NULL;
    }
    return (value_t*)do_get(cls->instance_properties, prop_name);
}
