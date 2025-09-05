#include "local_date.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "runtime_error.h"
#include "library_assert.h"

// External reference to global LocalDate class storage (declared in datetime.c)
extern value_t* global_local_date_class;

// LocalDate factory function
value_t local_date_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 3) {
        runtime_error(vm, "LocalDate() requires 3 arguments: year, month, day");
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error(vm, "LocalDate() arguments must be numbers");
    }
    
    int year = (int)value_to_float64(args[0]);
    int month = (int)value_to_float64(args[1]);
    int day = (int)value_to_float64(args[2]);
    
    // Validate date components
    if (!is_valid_date(year, month, day)) {
        runtime_error(vm, "Invalid date parameters");
    }
    
    local_date_t* date = local_date_create(NULL, year, month, day);
    if (!date) {
        if (g_current_vm) {
            slate_runtime_error(g_current_vm, ERR_OOM, __FILE__, __LINE__, -1, 
                               "Memory allocation failed");
        } else {
            fprintf(stderr, "Memory allocation failed\n");
            abort();
        }
    }
    return make_local_date(date);
}

// Static factory methods (used by the class initialization)

// LocalDate.now() - Built-in implementation already exists in datetime.c
// Referenced as builtin_local_date_now

// LocalDate.of() - Built-in implementation already exists in datetime.c  
// Referenced as builtin_local_date_of