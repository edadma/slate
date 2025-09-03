#ifndef SLATE_OPCODES_H
#define SLATE_OPCODES_H

#include "vm.h"

// Individual opcode implementations
vm_result op_add(vm_t* vm);
vm_result op_subtract(vm_t* vm);
vm_result op_multiply(vm_t* vm);
vm_result op_divide(vm_t* vm);
vm_result op_mod(vm_t* vm);
vm_result op_negate(vm_t* vm);
vm_result op_equal(vm_t* vm);
vm_result op_not_equal(vm_t* vm);
vm_result op_less(vm_t* vm);
vm_result op_less_equal(vm_t* vm);
vm_result op_greater(vm_t* vm);
vm_result op_greater_equal(vm_t* vm);
vm_result op_return(vm_t* vm);
vm_result op_get_local(vm_t* vm);
vm_result op_set_local(vm_t* vm);
vm_result op_define_global(vm_t* vm);
vm_result op_set_global(vm_t* vm);
vm_result op_build_object(vm_t* vm);
vm_result op_power(vm_t* vm);
vm_result op_build_array(vm_t* vm);
vm_result op_and(vm_t* vm);
vm_result op_bitwise_and(vm_t* vm);

// New opcodes extracted from vm.c
vm_result op_push_constant(vm_t* vm);
vm_result op_get_global(vm_t* vm);
vm_result op_get_property(vm_t* vm);
vm_result op_call(vm_t* vm);
vm_result op_closure(vm_t* vm);
vm_result op_set_debug_location(vm_t* vm);
vm_result op_push_null(vm_t* vm);
vm_result op_push_undefined(vm_t* vm);
vm_result op_push_true(vm_t* vm);
vm_result op_push_false(vm_t* vm);
vm_result op_pop(vm_t* vm);
vm_result op_dup(vm_t* vm);
vm_result op_set_result(vm_t* vm);
vm_result op_clear_debug_location(vm_t* vm);
vm_result op_halt(vm_t* vm);

// Missing opcodes causing test failures
vm_result op_bitwise_or(vm_t* vm);
vm_result op_bitwise_xor(vm_t* vm);
vm_result op_bitwise_not(vm_t* vm);
vm_result op_left_shift(vm_t* vm);
vm_result op_right_shift(vm_t* vm);
vm_result op_logical_right_shift(vm_t* vm);
vm_result op_floor_div(vm_t* vm);
vm_result op_increment(vm_t* vm);
vm_result op_decrement(vm_t* vm);
vm_result op_in(vm_t* vm);
vm_result op_call_method(vm_t* vm);
vm_result op_pop_n_preserve_top(vm_t* vm);
vm_result op_build_range(vm_t* vm);
vm_result op_jump_if_false(vm_t* vm);
vm_result op_jump(vm_t* vm);
vm_result op_loop(vm_t* vm);
vm_result op_pop_n(vm_t* vm);
vm_result op_or(vm_t* vm);
vm_result op_not(vm_t* vm);
vm_result op_null_coalesce(vm_t* vm);
vm_result op_instanceof(vm_t* vm);

// Module system opcodes
vm_result op_import_module(vm_t* vm);
vm_result op_get_export(vm_t* vm);

#endif // SLATE_OPCODES_H