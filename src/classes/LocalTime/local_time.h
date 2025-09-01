#ifndef SLATE_LOCAL_TIME_CLASS_H
#define SLATE_LOCAL_TIME_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// LocalTime Class Initialization
void local_time_class_init(slate_vm* vm);

// LocalTime Factory Function
value_t local_time_factory(value_t* args, int arg_count);

// LocalTime Instance Methods
value_t builtin_local_time_hour(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_minute(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_second(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_millisecond(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_hours(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_minutes(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_plus_seconds(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_hours(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_minutes(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_minus_seconds(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_equals(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_is_before(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_is_after(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_time_to_string(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_LOCAL_TIME_CLASS_H