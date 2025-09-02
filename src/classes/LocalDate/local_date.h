#ifndef SLATE_LOCAL_DATE_CLASS_H
#define SLATE_LOCAL_DATE_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;

// LocalDate Class Initialization
void local_date_class_init(vm_t* vm);

// LocalDate Factory Function
value_t local_date_factory(vm_t* vm, int arg_count, value_t* args);

// LocalDate Instance Methods
value_t builtin_local_date_year(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_month(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_day(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_day_of_week(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_day_of_year(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_days(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_months(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_years(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_days(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_months(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_years(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_is_before(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_is_after(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_to_string(vm_t* vm, int arg_count, value_t* args);

// LocalDate Static Methods are in datetime.c:
// - builtin_local_date_now
// - builtin_local_date_of

#endif // SLATE_LOCAL_DATE_CLASS_H