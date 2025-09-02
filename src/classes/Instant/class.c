#include "instant.h"
#include "value.h"
#include "vm.h"
#include "dynamic_object.h"

// Initialize Instant class with prototype and methods
void init_instant_class(vm_t* vm) {
    // Create the Instant class with its prototype
    do_object instant_proto = do_create(NULL);
    
    // Add instance methods to Instant prototype
    value_t to_epoch_milli_method = make_native(instant_to_epoch_milli);
    do_set(instant_proto, "toEpochMilli", &to_epoch_milli_method, sizeof(value_t));
    
    value_t to_epoch_second_method = make_native(instant_to_epoch_second);
    do_set(instant_proto, "toEpochSecond", &to_epoch_second_method, sizeof(value_t));
    
    value_t plus_millis_method = make_native(instant_plus_millis);
    do_set(instant_proto, "plusMillis", &plus_millis_method, sizeof(value_t));
    
    value_t minus_millis_method = make_native(instant_minus_millis);
    do_set(instant_proto, "minusMillis", &minus_millis_method, sizeof(value_t));
    
    value_t plus_seconds_method = make_native(instant_plus_seconds);
    do_set(instant_proto, "plusSeconds", &plus_seconds_method, sizeof(value_t));
    
    value_t minus_seconds_method = make_native(instant_minus_seconds);
    do_set(instant_proto, "minusSeconds", &minus_seconds_method, sizeof(value_t));
    
    value_t is_before_method = make_native(instant_is_before);
    do_set(instant_proto, "isBefore", &is_before_method, sizeof(value_t));
    
    value_t is_after_method = make_native(instant_is_after);
    do_set(instant_proto, "isAfter", &is_after_method, sizeof(value_t));
    
    value_t equals_method = make_native(instant_equals);
    do_set(instant_proto, "equals", &equals_method, sizeof(value_t));
    
    value_t to_string_method = make_native(instant_to_string);
    do_set(instant_proto, "toString", &to_string_method, sizeof(value_t));
    
    // Create the Instant class
    value_t instant_class = make_class("Instant", instant_proto);
    
    // Set the factory function to allow Instant constructor calls
    instant_class.as.class->factory = instant_factory;
    
    // Add static methods to the Instant class
    value_t now_method = make_native(instant_now);
    do_set(instant_class.as.class->properties, "now", &now_method, sizeof(value_t));
    
    value_t of_epoch_second_method = make_native(instant_of_epoch_second);
    do_set(instant_class.as.class->properties, "ofEpochSecond", &of_epoch_second_method, sizeof(value_t));
    
    value_t parse_method = make_native(instant_parse);
    do_set(instant_class.as.class->properties, "parse", &parse_method, sizeof(value_t));
    
    // Store in globals
    do_set(vm->globals, "Instant", &instant_class, sizeof(value_t));
    
    // Store a global reference for use in make_instant_direct
    static value_t instant_class_storage;
    instant_class_storage = vm_retain(instant_class);
    global_instant_class = &instant_class_storage;
}