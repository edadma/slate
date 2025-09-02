#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "datetime.h"
#include "instant.h"
#include "timezone.h"
#include "date.h"
#include "builtins.h"

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

int is_truthy(value_t value) {
    return !is_falsy(value);
}

int is_number(value_t value) { return value.type == VAL_INT32 || value.type == VAL_BIGINT || value.type == VAL_FLOAT32 || value.type == VAL_FLOAT64; }

// Generalized numeric comparison function
// Returns: -1 if a < b, 0 if a == b, 1 if a > b
int compare_numbers(value_t a, value_t b) {
    // Handle same-type comparisons first (most efficient)
    if (a.type == b.type) {
        switch (a.type) {
            case VAL_INT32:
                if (a.as.int32 < b.as.int32) return -1;
                if (a.as.int32 > b.as.int32) return 1;
                return 0;
                
            case VAL_BIGINT:
                if (di_lt(a.as.bigint, b.as.bigint)) return -1;
                if (di_gt(a.as.bigint, b.as.bigint)) return 1;
                return 0;
                
            case VAL_FLOAT32: {
                float fa = a.as.float32;
                float fb = b.as.float32;
                if (fa < fb) return -1;
                if (fa > fb) return 1;
                return 0;
            }
            
            case VAL_FLOAT64: {
                double da = a.as.float64;
                double db = b.as.float64;
                if (da < db) return -1;
                if (da > db) return 1;
                return 0;
            }
            
            default:
                return 0; // Should not happen for numeric types
        }
    }
    
    // Handle cross-type comparisons
    // Priority order: Check for any floating point first, then handle integer comparisons
    
    // If either is floating point, convert both to appropriate floating point type
    if (a.type == VAL_FLOAT64 || b.type == VAL_FLOAT64 ||
        a.type == VAL_FLOAT32 || b.type == VAL_FLOAT32) {
        
        // Use double for highest precision
        double a_val = (a.type == VAL_INT32)   ? (double)a.as.int32
                     : (a.type == VAL_BIGINT)  ? di_to_double(a.as.bigint)
                     : (a.type == VAL_FLOAT32) ? (double)a.as.float32
                                               : a.as.float64;
                                               
        double b_val = (b.type == VAL_INT32)   ? (double)b.as.int32
                     : (b.type == VAL_BIGINT)  ? di_to_double(b.as.bigint)
                     : (b.type == VAL_FLOAT32) ? (double)b.as.float32
                                               : b.as.float64;
        
        if (a_val < b_val) return -1;
        if (a_val > b_val) return 1;
        return 0;
    }
    
    // Both are integers (INT32 and/or BIGINT) - handle without floating point
    if ((a.type == VAL_INT32 || a.type == VAL_BIGINT) &&
        (b.type == VAL_INT32 || b.type == VAL_BIGINT)) {
        
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
        if (a_needs_release) di_release(&a_big);
        if (b_needs_release) di_release(&b_big);
        
        return result;
    }
    
    // Should not reach here for valid numeric types
    return 0;
}

int values_equal(value_t a, value_t b) {
    // Handle cross-type numeric comparisons
    if (is_number(a) && is_number(b)) {
        // Convert to common type for comparison
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            return a.as.int32 == b.as.int32;
        } else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            return di_eq(a.as.bigint, b.as.bigint);
        } else if (a.type == VAL_FLOAT32 && b.type == VAL_FLOAT32) {
            // IEEE 754: NaN is never equal to NaN
            float fa = a.as.float32, fb = b.as.float32;
            if (isnan(fa) || isnan(fb)) return 0;
            return fa == fb;
        } else if (a.type == VAL_FLOAT64 && b.type == VAL_FLOAT64) {
            // IEEE 754: NaN is never equal to NaN
            double da = a.as.float64, db = b.as.float64;
            if (isnan(da) || isnan(db)) return 0;
            return da == db;
        }
        // Cross-type comparisons - convert to common type
        // Convert to double for simplicity
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
            : (a.type == VAL_FLOAT32)        ? (double)a.as.float32
                                             : a.as.float64;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
            : (b.type == VAL_FLOAT32)        ? (double)b.as.float32
                                             : b.as.float64;
        // IEEE 754: NaN is never equal to NaN, Infinity equals Infinity across precisions
        if (isnan(a_val) || isnan(b_val)) return 0;
        return a_val == b_val;
    }

    if (a.type != b.type)
        return 0;

    switch (a.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return a.as.boolean == b.as.boolean;
    case VAL_STRING:
        if (a.as.string == NULL && b.as.string == NULL)
            return 1;
        if (a.as.string == NULL || b.as.string == NULL)
            return 0;
        return strcmp(a.as.string, b.as.string) == 0; // DS strings work with strcmp!
    case VAL_ARRAY:
        return a.as.array == b.as.array;
    case VAL_OBJECT:
        return a.as.object == b.as.object;
    case VAL_CLASS:
        return a.as.class == b.as.class; // Class identity comparison
    case VAL_BUFFER:
        if (a.as.buffer == b.as.buffer)
            return 1; // Same buffer reference
        if (a.as.buffer == NULL || b.as.buffer == NULL)
            return 0;
        return db_equals(a.as.buffer, b.as.buffer);
    case VAL_BUFFER_BUILDER:
        return a.as.builder == b.as.builder;
    case VAL_BUFFER_READER:
        return a.as.reader == b.as.reader;
    case VAL_FUNCTION:
        return a.as.function == b.as.function;
    case VAL_CLOSURE:
        return a.as.closure == b.as.closure;
    case VAL_NATIVE:
        return a.as.native == b.as.native;
    case VAL_LOCAL_DATE:
        if (a.as.local_date == b.as.local_date)
            return 1; // Same reference
        if (a.as.local_date == NULL || b.as.local_date == NULL)
            return 0;
        return local_date_equals(a.as.local_date, b.as.local_date);
    case VAL_LOCAL_TIME:
        if (a.as.local_time == b.as.local_time)
            return 1; // Same reference
        if (a.as.local_time == NULL || b.as.local_time == NULL)
            return 0;
        return local_time_equals(a.as.local_time, b.as.local_time);
    case VAL_LOCAL_DATETIME:
        if (a.as.local_datetime == NULL || b.as.local_datetime == NULL)
            return 0;
        return local_datetime_equals(a.as.local_datetime, b.as.local_datetime);
    case VAL_ZONE:
        // Compare timezone IDs
        return a.as.zone == b.as.zone || 
               (a.as.zone && b.as.zone && strcmp(timezone_get_id(a.as.zone), timezone_get_id(b.as.zone)) == 0);
    case VAL_DATE:
        if (a.as.date == NULL || b.as.date == NULL)
            return 0;
        return date_equals(a.as.date, b.as.date);
    case VAL_DURATION:
    case VAL_PERIOD:
        // For now, use reference equality for these types
        return a.as.duration == b.as.duration;
    case VAL_INSTANT:
        // Direct comparison of epoch milliseconds
        return a.as.instant_millis == b.as.instant_millis;
    default:
        return 0;
    }
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
    case VAL_FLOAT32:
        {
            float val = value.as.float32;
            if (isnan(val)) {
                printf("NaN");
            } else if (isinf(val)) {
                printf("%s", val > 0 ? "Infinity" : "-Infinity");
            } else {
                printf("%.7g", val);
            }
        }
        break;
    case VAL_FLOAT64:
        {
            double val = value.as.float64;
            if (isnan(val)) {
                printf("NaN");
            } else if (isinf(val)) {
                printf("%s", val > 0 ? "Infinity" : "-Infinity");
            } else {
                printf("%.6g", val);
            }
        }
        break;
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
        printf("{");
        if (value.as.object) {
            // For now, just show it's an object - full object printing would be more complex
            printf("Object");
        }
        printf("}");
        break;
    }
    case VAL_CLASS: {
        if (value.as.class) {
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
        printf("<Zone>");  // TODO: implement string conversion
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
        printf("<Duration>");  // TODO: implement string conversion
        break;
    }
    case VAL_PERIOD: {
        printf("<Period>");  // TODO: implement string conversion
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
        return value.as.float64 == floor(value.as.float64) && value.as.float64 >= INT_MIN && value.as.float64 <= INT_MAX;
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