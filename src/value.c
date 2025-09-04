#include "value.h"
#include "date.h"
#include <stdlib.h>
#include <string.h>

// debug_location is now properly defined in vm.h via date.h

// Memory management functions
value_t vm_retain(value_t value) {
    if (value.type == VAL_STRING) {
        value.as.string = ds_retain(value.as.string);
    } else if (value.type == VAL_STRING_BUILDER) {
        value.as.string_builder = ds_builder_retain(value.as.string_builder);
    } else if (value.type == VAL_ARRAY) {
        value.as.array = da_retain(value.as.array);
    } else if (value.type == VAL_OBJECT) {
        value.as.object = do_retain(value.as.object);
    } else if (value.type == VAL_CLASS) {
        value.as.class = class_retain(value.as.class);
    } else if (value.type == VAL_BIGINT) {
        value.as.bigint = di_retain(value.as.bigint);
    } else if (value.type == VAL_BUFFER) {
        value.as.buffer = db_retain(value.as.buffer);
    } else if (value.type == VAL_BUFFER_BUILDER) {
        value.as.builder = db_builder_retain(value.as.builder);
    } else if (value.type == VAL_BUFFER_READER) {
        value.as.reader = db_reader_retain(value.as.reader);
    } else if (value.type == VAL_RANGE) {
        value.as.range->ref_count++;
    } else if (value.type == VAL_ITERATOR) {
        value.as.iterator->ref_count++;
    } else if (value.type == VAL_BOUND_METHOD) {
        value.as.bound_method->ref_count++;
    } else if (value.type == VAL_LOCAL_DATE) {
        value.as.local_date->ref_count++;
    } else if (value.type == VAL_LOCAL_TIME) {
        value.as.local_time->ref_count++;
    } else if (value.type == VAL_LOCAL_DATETIME) {
        value.as.local_datetime->ref_count++;
    } else if (value.type == VAL_ZONE) {
        // Zones are managed by the timezone system, no reference counting
    } else if (value.type == VAL_DATE) {
        value.as.date->ref_count++;
    } else if (value.type == VAL_INSTANT) {
        // Direct storage - no reference counting needed
    } else if (value.type == VAL_DURATION) {
        value.as.duration->ref_count++;
    } else if (value.type == VAL_PERIOD) {
        value.as.period->ref_count++;
    }
    return value;
}

void vm_release(value_t value) {
    if (value.type == VAL_STRING) {
        ds_string temp = value.as.string;
        ds_release(&temp);
    } else if (value.type == VAL_STRING_BUILDER) {
        ds_builder temp = value.as.string_builder;
        ds_builder_release(&temp);
    } else if (value.type == VAL_ARRAY) {
        da_array temp = value.as.array;
        da_release(&temp);
    } else if (value.type == VAL_OBJECT) {
        do_object temp = value.as.object;
        do_release(&temp);
    } else if (value.type == VAL_CLASS) {
        class_release(value.as.class);
    } else if (value.type == VAL_BIGINT) {
        di_int temp = value.as.bigint;
        di_release(&temp);
    } else if (value.type == VAL_BUFFER) {
        db_buffer temp = value.as.buffer;
        db_release(&temp);
    } else if (value.type == VAL_BUFFER_BUILDER) {
        db_builder temp = value.as.builder;
        db_builder_release(&temp);
    } else if (value.type == VAL_BUFFER_READER) {
        db_reader temp = value.as.reader;
        db_reader_release(&temp);
    } else if (value.type == VAL_RANGE && value.as.range) {
        value.as.range->ref_count--;
        if (value.as.range->ref_count <= 0) {
            vm_release(value.as.range->start);
            vm_release(value.as.range->end);
            vm_release(value.as.range->step);
            free(value.as.range);
        }
    } else if (value.type == VAL_ITERATOR && value.as.iterator) {
        value.as.iterator->ref_count--;
        if (value.as.iterator->ref_count <= 0) {
            free_value(make_iterator(value.as.iterator));
        }
    } else if (value.type == VAL_BOUND_METHOD && value.as.bound_method) {
        bound_method_release(value.as.bound_method);
    } else if (value.type == VAL_LOCAL_DATE && value.as.local_date) {
        local_date_release(value.as.local_date);
    } else if (value.type == VAL_LOCAL_TIME && value.as.local_time) {
        local_time_release(value.as.local_time);
    } else if (value.type == VAL_LOCAL_DATETIME && value.as.local_datetime) {
        local_datetime_release(value.as.local_datetime);
    } else if (value.type == VAL_ZONE) {
        // Zones are managed by the timezone system, no release needed
    } else if (value.type == VAL_DATE && value.as.date) {
        date_release(value.as.date);
    } else if (value.type == VAL_INSTANT) {
        // Direct storage - no memory to release
    } else if (value.type == VAL_DURATION && value.as.duration) {
        duration_release(value.as.duration);
    } else if (value.type == VAL_PERIOD && value.as.period) {
        period_release(value.as.period);
    }
}

void free_value(value_t value) {
    vm_release(value);
}

// Basic value creation functions
value_t make_null(void) {
    value_t value;
    value.type = VAL_NULL;
    value.class = global_null_class; // All nulls have Null class
    value.debug = NULL;
    return value;
}

value_t make_undefined(void) {
    value_t value;
    value.type = VAL_UNDEFINED;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_boolean(int bool_val) {
    value_t value;
    value.type = VAL_BOOLEAN;
    value.as.boolean = bool_val;
    value.class = global_boolean_class; // All booleans have Boolean class
    value.debug = NULL;
    return value;
}

value_t make_int32(int32_t int_val) {
    value_t value;
    value.type = VAL_INT32;
    value.as.int32 = int_val;
    value.class = global_int_class; // All integers have Int class
    value.debug = NULL;
    return value;
}

value_t make_bigint(di_int bigint) {
    value_t value;
    value.type = VAL_BIGINT;
    value.as.bigint = bigint;
    value.class = global_int_class; // All integers have Int class
    value.debug = NULL;
    return value;
}

value_t make_float32(float float_val) {
    value_t value;
    value.type = VAL_FLOAT32;
    value.as.float32 = float_val;
    value.class = global_float_class; // Use Float class for float32
    value.debug = NULL;
    return value;
}

value_t make_float64(double double_val) {
    value_t value;
    value.type = VAL_FLOAT64;
    value.as.float64 = double_val;
    value.class = global_float_class; // Use Float class for float64
    value.debug = NULL;
    return value;
}

value_t make_string(const char* str) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = ds_new(str);
    value.class = global_string_class; // All strings have String class
    value.debug = NULL;
    return value;
}

value_t make_string_ds(ds_string string) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = string;
    value.class = global_string_class; // All strings have String class
    value.debug = NULL;
    return value;
}

value_t make_string_builder(ds_builder builder) {
    value_t value;
    value.type = VAL_STRING_BUILDER;
    value.as.string_builder = builder;
    value.class = global_string_builder_class; // All string builders have StringBuilder class
    value.debug = NULL;
    return value;
}

value_t make_array(da_array array) {
    value_t value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    value.class = global_array_class; // All arrays have Array class
    value.debug = NULL;
    return value;
}

value_t make_object(do_object object) {
    value_t value;
    value.type = VAL_OBJECT;
    value.as.object = object;
    value.class = global_object_class; // Objects inherit from Object class
    value.debug = NULL;
    return value;
}

value_t make_class(const char* name, do_object instance_properties, do_object static_properties) {
    class_t* cls = malloc(sizeof(class_t));
    if (!cls) {
        return make_null(); // Return null on allocation failure
    }

    cls->ref_count = 1;
    cls->name = strdup(name ? name : "Class"); // Duplicate the name string
    cls->instance_properties = instance_properties ? do_retain(instance_properties) : do_create(NULL); // Retain or create empty
    cls->static_properties = static_properties ? do_retain(static_properties) : do_create(NULL); // Retain or create empty
    cls->factory = NULL; // Default: class cannot be instantiated by calling it

    value_t value;
    value.type = VAL_CLASS;
    value.as.class = cls;
    value.class = global_value_class; // Classes inherit from Value
    value.debug = NULL;
    return value;
}

value_t make_range(value_t start, value_t end, int exclusive, value_t step) {
    range_t* range = malloc(sizeof(range_t));
    if (!range) {
        return make_null(); // Handle allocation failure
    }

    range->ref_count = 1; // Initialize reference count
    range->start = vm_retain(start);
    range->end = vm_retain(end);
    range->exclusive = exclusive;
    range->step = vm_retain(step);

    value_t value;
    value.type = VAL_RANGE;
    value.as.range = range;
    value.class = global_range_class; // All ranges have Range class
    value.debug = NULL;
    return value;
}

value_t make_iterator(iterator_t* iterator) {
    value_t value;
    value.type = VAL_ITERATOR;
    value.as.iterator = iterator;
    value.class = global_iterator_class; // All iterators have Iterator class
    value.debug = NULL;
    return value;
}

value_t make_function(struct function* function) {
    value_t value;
    value.type = VAL_FUNCTION;
    value.as.function = function;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_closure(struct closure* closure) {
    value_t value;
    value.type = VAL_CLOSURE;
    value.as.closure = closure;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_native(native_t native) {
    value_t value;
    value.type = VAL_NATIVE;
    value.as.native = native;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_bound_method(value_t receiver, native_t method_func) {
    bound_method_t* method = malloc(sizeof(bound_method_t));
    if (!method) {
        return make_null(); // Handle allocation failure
    }

    method->ref_count = 1; // Initialize reference count
    method->receiver = vm_retain(receiver);
    method->method = method_func;

    value_t value;
    value.type = VAL_BOUND_METHOD;
    value.as.bound_method = method;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer(db_buffer buffer) {
    value_t value;
    value.type = VAL_BUFFER;
    value.as.buffer = buffer;
    value.class = global_buffer_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer_builder(db_builder builder) {
    value_t value;
    value.type = VAL_BUFFER_BUILDER;
    value.as.builder = builder;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer_reader(db_reader reader) {
    value_t value;
    value.type = VAL_BUFFER_READER;
    value.as.reader = reader;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_local_date(local_date_t* date) {
    value_t value;
    value.type = VAL_LOCAL_DATE;
    value.as.local_date = date;
    value.class = global_local_date_class;
    value.debug = NULL;
    return value;
}

value_t make_local_time(local_time_t* time) {
    value_t value;
    value.type = VAL_LOCAL_TIME;
    value.as.local_time = time;
    value.class = global_local_time_class;
    value.debug = NULL;
    return value;
}

value_t make_local_datetime(local_datetime_t* datetime) {
    value_t value;
    value.type = VAL_LOCAL_DATETIME;
    value.as.local_datetime = datetime;
    value.class = global_local_datetime_class;
    value.debug = NULL;
    return value;
}

value_t make_zone(const timezone_t* timezone) {
    value_t value;
    value.type = VAL_ZONE;
    value.as.zone = timezone;
    value.class = global_zone_class;
    value.debug = NULL;
    return value;
}

value_t make_date(date_t* date) {
    value_t value;
    value.type = VAL_DATE;
    value.as.date = date;
    value.class = global_date_class;
    value.debug = NULL;
    return value;
}

value_t make_instant_direct(int64_t epoch_millis) {
    value_t value;
    value.type = VAL_INSTANT;
    value.as.instant_millis = epoch_millis;
    value.class = global_instant_class;
    value.debug = NULL;
    return value;
}

value_t make_duration(duration_t* duration) {
    value_t value;
    value.type = VAL_DURATION;
    value.as.duration = duration;
    value.class = global_duration_class;
    value.debug = NULL;
    return value;
}

value_t make_period(period_t* period) {
    value_t value;
    value.type = VAL_PERIOD;
    value.as.period = period;
    value.class = global_period_class;
    value.debug = NULL;
    return value;
}

// Value creation functions with debug info (copy debug location)
static debug_location* copy_debug_location(debug_location* original) {
    if (!original) return NULL;
    debug_location* copy = malloc(sizeof(debug_location));
    if (copy) {
        copy->line = original->line;
        copy->column = original->column;
        copy->source_text = original->source_text; // Shallow copy of source text
    }
    return copy;
}

value_t make_null_with_debug(debug_location* debug) {
    value_t value = make_null();
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_undefined_with_debug(debug_location* debug) {
    value_t value = make_undefined();
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_boolean_with_debug(int bool_val, debug_location* debug) {
    value_t value = make_boolean(bool_val);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_int32_with_debug(int32_t int_val, debug_location* debug) {
    value_t value = make_int32(int_val);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_bigint_with_debug(di_int bigint, debug_location* debug) {
    value_t value = make_bigint(bigint);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_float32_with_debug(float float_val, debug_location* debug) {
    value_t value = make_float32(float_val);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_float64_with_debug(double double_val, debug_location* debug) {
    value_t value = make_float64(double_val);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_string_with_debug(const char* str, debug_location* debug) {
    value_t value = make_string(str);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_string_ds_with_debug(ds_string string, debug_location* debug) {
    value_t value = make_string_ds(string);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_string_builder_with_debug(ds_builder builder, debug_location* debug) {
    value_t value = make_string_builder(builder);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_array_with_debug(da_array array, debug_location* debug) {
    value_t value = make_array(array);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_object_with_debug(do_object object, debug_location* debug) {
    value_t value = make_object(object);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_class_with_debug(const char* name, do_object instance_properties, do_object static_properties, debug_location* debug) {
    value_t value = make_class(name, instance_properties, static_properties);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_range_with_debug(value_t start, value_t end, int exclusive, value_t step, debug_location* debug) {
    value_t value = make_range(start, end, exclusive, step);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_iterator_with_debug(iterator_t* iterator, debug_location* debug) {
    value_t value = make_iterator(iterator);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_function_with_debug(struct function* function, debug_location* debug) {
    value_t value = make_function(function);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_closure_with_debug(struct closure* closure, debug_location* debug) {
    value_t value = make_closure(closure);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_native_with_debug(native_t native, debug_location* debug) {
    value_t value = make_native(native);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_bound_method_with_debug(value_t receiver, native_t method, debug_location* debug) {
    value_t value = make_bound_method(receiver, method);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_buffer_with_debug(db_buffer buffer, debug_location* debug) {
    value_t value = make_buffer(buffer);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_buffer_builder_with_debug(db_builder builder, debug_location* debug) {
    value_t value = make_buffer_builder(builder);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_buffer_reader_with_debug(db_reader reader, debug_location* debug) {
    value_t value = make_buffer_reader(reader);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_local_date_with_debug(local_date_t* date, debug_location* debug) {
    value_t value = make_local_date(date);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_local_time_with_debug(local_time_t* time, debug_location* debug) {
    value_t value = make_local_time(time);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_local_datetime_with_debug(local_datetime_t* datetime, debug_location* debug) {
    value_t value = make_local_datetime(datetime);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_zone_with_debug(const timezone_t* timezone, debug_location* debug) {
    value_t value = make_zone(timezone);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_date_with_debug(date_t* date, debug_location* debug) {
    value_t value = make_date(date);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_instant_direct_with_debug(int64_t epoch_millis, debug_location* debug) {
    value_t value = make_instant_direct(epoch_millis);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_duration_with_debug(duration_t* duration, debug_location* debug) {
    value_t value = make_duration(duration);
    value.debug = copy_debug_location(debug);
    return value;
}

value_t make_period_with_debug(period_t* period, debug_location* debug) {
    value_t value = make_period(period);
    value.debug = copy_debug_location(debug);
    return value;
}

// Class utility functions
class_t* class_retain(class_t* class) {
    if (class) {
        class->ref_count++;
    }
    return class;
}

void class_release(class_t* class) {
    if (class) {
        class->ref_count--;
        if (class->ref_count <= 0) {
            free(class->name); // Free the strdup'd name
            do_object temp_instance_props = class->instance_properties;
            do_release(&temp_instance_props);
            do_object temp_static_props = class->static_properties;
            do_release(&temp_static_props);
            free(class);
        }
    }
}