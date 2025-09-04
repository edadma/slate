#include "zone.h"
#include "value.h"
#include "vm.h"
#include "dynamic_object.h"

// Global reference to Zone class (declared in value.h)
value_t* global_zone_class = NULL;

// Initialize Zone class with prototype and methods
void init_zone_class(vm_t* vm) {
    // Create the Zone class with its prototype
    do_object zone_proto = do_create(NULL);
    
    // Add instance methods to Zone prototype
    value_t id_method = make_native(zone_id);
    do_set(zone_proto, "id", &id_method, sizeof(value_t));
    
    value_t offset_method = make_native(zone_offset);
    do_set(zone_proto, "offset", &offset_method, sizeof(value_t));
    
    value_t is_dst_method = make_native(zone_is_dst);
    do_set(zone_proto, "isDst", &is_dst_method, sizeof(value_t));
    
    value_t display_name_method = make_native(zone_display_name);
    do_set(zone_proto, "displayName", &display_name_method, sizeof(value_t));
    
    value_t equals_method = make_native(zone_equals);
    do_set(zone_proto, "equals", &equals_method, sizeof(value_t));
    
    value_t to_string_method = make_native(zone_to_string);
    do_set(zone_proto, "toString", &to_string_method, sizeof(value_t));
    
    // Create static methods object
    do_object zone_static = do_create(NULL);
    value_t utc_method = make_native(zone_utc);
    do_set(zone_static, "utc", &utc_method, sizeof(value_t));
    
    value_t system_method = make_native(zone_system);
    do_set(zone_static, "system", &system_method, sizeof(value_t));
    
    value_t of_method = make_native(zone_of);
    do_set(zone_static, "of", &of_method, sizeof(value_t));
    
    // Create the Zone class
    value_t zone_class = make_class("Zone", zone_proto, zone_static);
    
    // Set the factory function to allow Zone constructor calls
    zone_class.as.class->factory = zone_factory;
    
    // Store in globals
    do_set(vm->globals, "Zone", &zone_class, sizeof(value_t));
    
    // Store a global reference for use in make_zone_direct
    static value_t zone_class_storage;
    zone_class_storage = vm_retain(zone_class);
    global_zone_class = &zone_class_storage;
}

// Utility functions for creating Zone values
value_t make_zone_direct(const timezone_t* timezone) {
    return make_zone(timezone);
}

value_t make_zone_direct_with_debug(const timezone_t* timezone, debug_location* debug) {
    return make_zone_with_debug(timezone, debug);
}