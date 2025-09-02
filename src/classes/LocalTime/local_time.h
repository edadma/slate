#ifndef SLATE_LOCAL_TIME_CLASS_H
#define SLATE_LOCAL_TIME_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;

// LocalTime Class Initialization
void local_time_class_init(vm_t* vm);

// LocalTime Factory Function
value_t local_time_factory(vm_t* vm, int arg_count, value_t* args);

// LocalTime Instance Methods
value_t builtin_local_time_hour(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_minute(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_second(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_millisecond(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_hours(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_minutes(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_hours(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_minutes(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_is_before(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_is_after(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_to_string(vm_t* vm, int arg_count, value_t* args);

#endif // SLATE_LOCAL_TIME_CLASS_H