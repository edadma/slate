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
    
    // Then check class prototype chain
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
    // Call .equals() method on the left operand using method dispatch
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
    switch (value.type) {
    case VAL_NULL:
        printf("null");
        break;
    case VAL_UNDEFINED:
        printf("undefined");
        break;
    case VAL_BOOLEAN:
        printf(value.as.boolean ? "true" : "false");
        break;
    case VAL_INT32:
        printf("%d", value.as.int32);
        break;
    case VAL_BIGINT: {
        char* str = di_to_string(value.as.bigint, 10);
        if (str) {
            printf("%s", str);
            free(str);
        } else {
            printf("<bigint>");
        }
        break;
    }
    case VAL_FLOAT32: {
        float val = value.as.float32;
        if (isnan(val)) {
            printf("NaN");
        } else if (isinf(val)) {
            printf("%s", val > 0 ? "Infinity" : "-Infinity");
        } else {
            printf("%.7g", val);
        }
    } break;
    case VAL_FLOAT64: {
        double val = value.as.float64;
        if (isnan(val)) {
            printf("NaN");
        } else if (isinf(val)) {
            printf("%s", val > 0 ? "Infinity" : "-Infinity");
        } else {
            printf("%.6g", val);
        }
    } break;
    case VAL_STRING:
        printf("\"%s\"", value.as.string ? value.as.string : ""); // DS strings work directly!
        break;
    case VAL_STRING_BUILDER: {
        printf("<StringBuilder>");
        break;
    }
    case VAL_ARRAY: {
        printf("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0)
                    printf(", ");
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    print_value(vm, *element);
                } else {
                    printf("null");
                }
            }
        }
        printf("]");
        break;
    }
    case VAL_OBJECT: {
        if (value.as.object) {
            // Try to call .toString() method first
            value_t toString_result = call_toString_method(vm, value);
            if (toString_result.type == VAL_STRING) {
                printf("%s", toString_result.as.string);
                vm_release(toString_result);
            } else {
                // Fallback to generic object representation
                printf("{Object}");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_CLASS: {
        if (value.as.class) {
            // Try to call class-level toString method first
            if (value.as.class->static_properties) {
                value_t* toString_method = (value_t*)do_get(value.as.class->static_properties, "toString");
                if (toString_method && toString_method->type == VAL_NATIVE) {
                    value_t args[1] = { value };
                    native_t native_func = (native_t)toString_method->as.native;
                    value_t result = native_func(vm, 1, args);
                    
                    if (result.type == VAL_STRING) {
                        printf("%s", result.as.string);
                        vm_release(result);
                        break;
                    }
                    vm_release(result);
                }
            }
            
            // Fallback to default class representation
            printf("<class %s>", value.as.class->name ? value.as.class->name : "anonymous");
        } else {
            printf("<null class>");
        }
        break;
    }
    case VAL_FUNCTION:
        printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
        break;
    case VAL_CLOSURE:
        printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
        break;
    case VAL_NATIVE:
        printf("<builtin function>");
        break;
    case VAL_RANGE: {
        if (!value.as.range) {
            printf("<null range>");
        } else {
            print_value(vm, value.as.range->start);
            printf(value.as.range->exclusive ? "..<" : "..");
            print_value(vm, value.as.range->end);
        }
        break;
    }
    case VAL_ITERATOR: {
        if (!value.as.iterator) {
            printf("<null iterator>");
        } else if (value.as.iterator->type == ITER_ARRAY) {
            printf("<array iterator>");
        } else if (value.as.iterator->type == ITER_RANGE) {
            printf("<range iterator>");
        } else {
            printf("<unknown iterator>");
        }
        break;
    }
    case VAL_BUFFER: {
        if (!value.as.buffer) {
            printf("<null buffer>");
        } else {
            printf("<buffer size=%zu>", db_size(value.as.buffer));
        }
        break;
    }
    case VAL_BUFFER_BUILDER: {
        if (!value.as.builder) {
            printf("<null buffer builder>");
        } else {
            printf("<buffer builder>");
        }
        break;
    }
    case VAL_BUFFER_READER: {
        if (!value.as.reader) {
            printf("<null buffer reader>");
        } else {
            printf("<buffer reader pos=%zu>", db_reader_position(value.as.reader));
        }
        break;
    }
    case VAL_BOUND_METHOD: {
        if (!value.as.bound_method) {
            printf("<null bound method>");
        } else {
            printf("<bound method>");
        }
        break;
    }
    case VAL_LOCAL_DATE: {
        if (value.as.local_date) {
            char* str = local_date_to_string(vm, value.as.local_date);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDate>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_TIME: {
        if (value.as.local_time) {
            char* str = local_time_to_string(vm, value.as.local_time);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_DATETIME: {
        if (value.as.local_datetime) {
            char* str = local_datetime_to_string(vm, value.as.local_datetime);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDateTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_ZONE: {
        printf("<Zone>"); // TODO: implement string conversion
        break;
    }
    case VAL_DATE: {
        if (value.as.date) {
            char* str = date_to_iso_string(vm, value.as.date);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<Date>");
            }
        } else {
            printf("<Date>");
        }
        break;
    }
    case VAL_INSTANT: {
        // Use the toString method for proper ISO 8601 formatting
        value_t str_result = instant_to_string(vm, 1, &value);
        if (str_result.type == VAL_STRING) {
            printf("%s", str_result.as.string);
            vm_release(str_result);
        } else {
            printf("<Instant:%ld>", value.as.instant_millis);
        }
        break;
    }
    case VAL_DURATION: {
        printf("<Duration>"); // TODO: implement string conversion
        break;
    }
    case VAL_PERIOD: {
        printf("<Period>"); // TODO: implement string conversion
        break;
    }
    }
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
