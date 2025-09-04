#ifndef SLATE_VALUE_H
#define SLATE_VALUE_H

#include <stdint.h>
// Include dynamic libraries
#include "dynamic_array.h"
#include "dynamic_buffer.h"
#include "dynamic_int.h"
#include "dynamic_object.h"
#include "dynamic_string.h"

// Forward declarations to avoid circular includes
typedef struct slate_vm vm_t;
struct debug_location;
typedef struct debug_location debug_location;

// VM value types
typedef enum {
    VAL_NULL,
    VAL_UNDEFINED,
    VAL_BOOLEAN,
    VAL_INT32, // 32-bit integer (MCU-friendly default)
    VAL_BIGINT, // Arbitrary precision integer
    VAL_FLOAT32, // Single precision floating point (32-bit)
    VAL_FLOAT64, // Double precision floating point (64-bit)
    VAL_STRING,
    VAL_STRING_BUILDER, // String builder for constructing strings
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_CLASS, // Class definition (prototype holder)
    VAL_RANGE, // Range object (1..10, 1..<10)
    VAL_ITERATOR, // Iterator object for arrays, ranges, etc.
    VAL_BUFFER, // Byte buffer for binary data
    VAL_BUFFER_BUILDER, // Buffer builder for constructing buffers
    VAL_BUFFER_READER, // Buffer reader for parsing buffers
    VAL_FUNCTION,
    VAL_CLOSURE,
    VAL_NATIVE,
    VAL_BOUND_METHOD,
    // Date/Time types
    VAL_LOCAL_DATE, // Date without time zone (2024-12-25)
    VAL_LOCAL_TIME, // Time without date or time zone (15:30:45)
    VAL_LOCAL_DATETIME, // Date and time without time zone (2024-12-25T15:30:45)
    VAL_ZONE, // Timezone information (America/Toronto, UTC, etc.)
    VAL_DATE, // Date and time with timezone (the primary zoned datetime type)
    VAL_INSTANT, // Point in time (Unix timestamp with nanoseconds)
    VAL_DURATION, // Time-based amount (2 hours, 30 minutes)
    VAL_PERIOD // Date-based amount (2 years, 3 months, 5 days)
} value_type;

// Forward declarations for value-related structures
typedef struct value value_t;
typedef struct range range_t;
typedef struct iterator iterator_t;
typedef struct bound_method bound_method_t;
typedef struct class class_t;
typedef struct local_date local_date_t;
typedef struct local_time local_time_t;
typedef struct local_datetime local_datetime_t;
typedef struct timezone timezone_t;
typedef struct date date_t;
typedef struct instant instant_t;
typedef struct duration duration_t;
typedef struct period period_t;

// Native function pointer type
typedef value_t (*native_t)(vm_t* vm, int arg_count, value_t* args);

// VM value structure
struct value {
    value_type type;
    union {
        int boolean;
        int32_t int32; // 32-bit integer (direct storage)
        di_int bigint; // Arbitrary precision integer (ref-counted)
        float float32; // Single precision floating point
        double float64; // Double precision floating point
        ds_string string; // Using dynamic_string.h!
        ds_builder string_builder; // String builder (using dynamic_string.h!)
        da_array array; // Using dynamic_array.h!
        do_object object; // Using dynamic_object.h!
        class_t* class; // Class definition pointer
        range_t* range; // Range object pointer
        iterator_t* iterator; // Iterator object pointer
        db_buffer buffer; // Buffer data (using dynamic_buffer.h!)
        db_builder builder; // Buffer builder handle (reference counted)
        db_reader reader; // Buffer reader handle (reference counted)
        struct function* function;
        struct closure* closure;
        native_t native; // Native C function pointer
        bound_method_t* bound_method; // Bound method (method + receiver)
        // Date/Time types
        local_date_t* local_date; // Date without time zone
        local_time_t* local_time; // Time without date or time zone
        local_datetime_t* local_datetime; // Date and time without time zone
        const timezone_t* zone; // Timezone information (pointer to timezone data)
        date_t* date; // Date and time with timezone (zoned datetime)
        int64_t instant_millis; // Point in time (epoch milliseconds, direct storage)
        duration_t* duration; // Time-based amount
        period_t* period; // Date-based amount
    } as;
    value_t* class; // For object instances: pointer to their class value (NULL for non-instances)
    debug_location* debug; // Debug info for error reporting (NULL when disabled)
};

// Range structure for range expressions (1..10, 1..<10, 1..10 step 2)
struct range {
    int ref_count; // Reference count for memory management
    value_t start; // Starting value
    value_t end; // Ending value
    int exclusive; // 1 for ..< (exclusive), 0 for .. (inclusive)
    value_t step; // Step value (default: INT32(1) or INT32(-1) for auto-detected direction)
};

// Iterator types
typedef enum {
    ITER_ARRAY, // Array iterator
    ITER_RANGE // Range iterator
} iterator_type;

// Iterator structure for unified iteration over arrays, ranges, etc.
struct iterator {
    size_t ref_count; // Reference counting for memory management
    iterator_type type;
    union {
        struct {
            da_array array; // Array being iterated
            size_t index; // Current index
        } array_iter;
        struct {
            value_t current; // Current value
            value_t end; // End value
            value_t step; // Step value for iteration
            int exclusive; // Whether end is exclusive
            int finished; // Whether iteration is complete
            int reverse; // 1 if iterating backwards (start > end), 0 if forwards
        } range_iter;
    } data;
};

// Bound method structure
struct bound_method {
    size_t ref_count; // Reference counting for memory management
    value_t receiver; // Object instance
    native_t method; // Method function pointer
};

// Class structure
struct class {
    size_t ref_count; // Reference count for memory management
    char* name; // Class name (owned string)
    do_object instance_properties; // Hash table of instance methods/prototype properties
    do_object static_properties; // Hash table of static methods/class properties
    value_t (*factory)(vm_t* vm, int arg_count,
                       value_t* args); // Factory function for creating instances (NULL if not callable)
};

// Date/Time structures (forward declared, implemented in datetime.c)
struct local_date {
    size_t ref_count;
    int year;
    int month;
    int day;
    uint32_t epoch_day; // Days since epoch (used for calculations)
};

struct local_time {
    size_t ref_count;
    int hour;
    int minute;
    int second;
    int millis; // Milliseconds component
    int64_t nanos; // Total nanoseconds since midnight
};

struct local_datetime {
    size_t ref_count;
    local_date_t* date; // Pointer to local_date (retained)
    local_time_t* time; // Pointer to local_time (retained)
};

struct zoned_datetime {
    size_t ref_count;
    local_datetime_t* dt; // Pointer to local_datetime (retained)
    char* zone_id; // Time zone ID (owned string)
    int offset_seconds; // Offset from UTC in seconds
};

struct instant {
    size_t ref_count;
    int64_t seconds;
    int32_t nanoseconds;
};

struct duration {
    size_t ref_count;
    int64_t seconds;
    int32_t nanoseconds;
};

struct period {
    size_t ref_count;
    int years;
    int months;
    int days;
};

// Global class instances (extern declarations)
extern value_t* global_value_class;
extern value_t* global_object_class;
extern value_t* global_int_class;
extern value_t* global_float_class;
extern value_t* global_string_class;
extern value_t* global_boolean_class;
extern value_t* global_null_class;
extern value_t* global_array_class;
extern value_t* global_range_class;
extern value_t* global_iterator_class;
extern value_t* global_string_builder_class;
extern value_t* global_buffer_class;
extern value_t* global_buffer_builder_class;
extern value_t* global_local_date_class;
extern value_t* global_local_time_class;
extern value_t* global_local_datetime_class;
extern value_t* global_zone_class;
extern value_t* global_date_class;
extern value_t* global_instant_class;
extern value_t* global_duration_class;
extern value_t* global_period_class;

// Memory management functions
value_t vm_retain(value_t value);
void vm_release(value_t value);
void free_value(value_t value);

// Value creation functions (basic versions without debug info)
value_t make_null(void);
value_t make_undefined(void);
value_t make_boolean(int value);
value_t make_int32(int32_t value);
value_t make_bigint(di_int bigint);
value_t make_float32(float value);
value_t make_float64(double value);
value_t make_string(const char* value);
value_t make_string_ds(ds_string string);
value_t make_string_builder(ds_builder builder);
value_t make_array(da_array array);
value_t make_object(do_object object);
value_t make_class(const char* name, do_object instance_properties, do_object static_properties);
value_t make_range(value_t start, value_t end, int exclusive, value_t step);
value_t make_iterator(iterator_t* iterator);
value_t make_function(struct function* function);
value_t make_closure(struct closure* closure);
value_t make_native(native_t native);
value_t make_bound_method(value_t receiver, native_t method);
value_t make_buffer(db_buffer buffer);
value_t make_buffer_builder(db_builder builder);
value_t make_buffer_reader(db_reader reader);
value_t make_local_date(local_date_t* date);
value_t make_local_time(local_time_t* time);
value_t make_local_datetime(local_datetime_t* datetime);
value_t make_zone(const timezone_t* timezone);
value_t make_date(date_t* date);
value_t make_instant_direct(int64_t epoch_millis);
value_t make_duration(duration_t* duration);
value_t make_period(period_t* period);

// Value creation functions with debug info
value_t make_null_with_debug(debug_location* debug);
value_t make_undefined_with_debug(debug_location* debug);
value_t make_boolean_with_debug(int value, debug_location* debug);
value_t make_int32_with_debug(int32_t value, debug_location* debug);
value_t make_bigint_with_debug(di_int bigint, debug_location* debug);
value_t make_float32_with_debug(float value, debug_location* debug);
value_t make_float64_with_debug(double value, debug_location* debug);
value_t make_string_with_debug(const char* value, debug_location* debug);
value_t make_string_ds_with_debug(ds_string string, debug_location* debug);
value_t make_string_builder_with_debug(ds_builder builder, debug_location* debug);
value_t make_array_with_debug(da_array array, debug_location* debug);
value_t make_object_with_debug(do_object object, debug_location* debug);
value_t make_class_with_debug(const char* name, do_object instance_properties, do_object static_properties, debug_location* debug);
value_t make_range_with_debug(value_t start, value_t end, int exclusive, value_t step, debug_location* debug);
value_t make_iterator_with_debug(iterator_t* iterator, debug_location* debug);
value_t make_function_with_debug(struct function* function, debug_location* debug);
value_t make_closure_with_debug(struct closure* closure, debug_location* debug);
value_t make_native_with_debug(native_t native, debug_location* debug);
value_t make_bound_method_with_debug(value_t receiver, native_t method, debug_location* debug);
value_t make_buffer_with_debug(db_buffer buffer, debug_location* debug);
value_t make_buffer_builder_with_debug(db_builder builder, debug_location* debug);
value_t make_buffer_reader_with_debug(db_reader reader, debug_location* debug);
value_t make_local_date_with_debug(local_date_t* date, debug_location* debug);
value_t make_local_time_with_debug(local_time_t* time, debug_location* debug);
value_t make_local_datetime_with_debug(local_datetime_t* datetime, debug_location* debug);
value_t make_zone_with_debug(const timezone_t* timezone, debug_location* debug);
value_t make_date_with_debug(date_t* date, debug_location* debug);
value_t make_instant_direct_with_debug(int64_t epoch_millis, debug_location* debug);
value_t make_duration_with_debug(duration_t* duration, debug_location* debug);
value_t make_period_with_debug(period_t* period, debug_location* debug);

// Utility functions for classes
class_t* class_retain(class_t* class);
void class_release(class_t* class);

// Reference counting functions for other types (declared here to avoid circular includes)
void bound_method_release(bound_method_t* method);
void local_date_release(local_date_t* date);
void local_time_release(local_time_t* time);
void local_datetime_release(local_datetime_t* dt);
// zoned_datetime functions removed - use Date class instead
void instant_release(instant_t* instant);
void duration_release(duration_t* duration);
void period_release(period_t* period);

#endif // SLATE_VALUE_H
