#ifndef SLATE_LOCAL_DATE_CLASS_H
#define SLATE_LOCAL_DATE_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// LocalDate Class Initialization
void local_date_class_init(slate_vm* vm);

// LocalDate Factory Function
value_t local_date_factory(value_t* args, int arg_count);

// LocalDate Instance Methods
value_t builtin_local_date_year(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_month(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_day(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_day_of_week(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_day_of_year(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_days(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_months(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_plus_years(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_days(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_months(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_minus_years(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_equals(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_is_before(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_is_after(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_local_date_to_string(slate_vm* vm, int arg_count, value_t* args);

// LocalDate Static Methods are in datetime.c:
// - builtin_local_date_now
// - builtin_local_date_of

#endif // SLATE_LOCAL_DATE_CLASS_H