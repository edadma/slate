#include "vm.h"
#include "module.h"
#include "../opcodes/opcodes.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <stdio.h>
#include <assert.h>

// Core VM execution loop - runs until completion
// Assumes VM is already set up with proper call frames and stack
vm_result vm_run(vm_t* vm) {
    if (!vm)
        return VM_RUNTIME_ERROR;

    // Main execution loop
    for (;;) {
        vm->current_instruction = vm->ip; // Store instruction start for error reporting
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            vm_result result = op_push_constant(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_NULL: {
            vm_result result = op_push_null(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_UNDEFINED: {
            vm_result result = op_push_undefined(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_TRUE: {
            vm_result result = op_push_true(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_PUSH_FALSE: {
            vm_result result = op_push_false(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POP: {
            vm_result result = op_pop(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DUP: {
            vm_result result = op_dup(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_RESULT: {
            vm_result result = op_set_result(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_ADD: {
            vm_result result = op_add(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SUBTRACT: {
            vm_result result = op_subtract(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MULTIPLY: {
            vm_result result = op_multiply(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DIVIDE: {
            vm_result result = op_divide(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NEGATE: {
            vm_result result = op_negate(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MOD: {
            vm_result result = op_mod(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POWER: {
            vm_result result = op_power(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_EQUAL: {
            vm_result result = op_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NOT_EQUAL: {
            vm_result result = op_not_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_AND: {
            vm_result result = op_and(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_OR: {
            vm_result result = op_or(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NULL_COALESCE: {
            vm_result result = op_null_coalesce(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_INSTANCEOF: {
            vm_result result = op_instanceof(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NOT: {
            vm_result result = op_not(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LESS: {
            vm_result result = op_less(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GREATER: {
            vm_result result = op_greater(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LESS_EQUAL: {
            vm_result result = op_less_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GREATER_EQUAL: {
            vm_result result = op_greater_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_RETURN: {
            vm_result result = op_return(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_LOCAL: {
            vm_result result = op_get_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_LOCAL: {
            vm_result result = op_set_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_GLOBAL: {
            vm_result result = op_get_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DEFINE_GLOBAL: {
            vm_result result = op_define_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_GLOBAL: {
            vm_result result = op_set_global(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_PROPERTY: {
            vm_result result = op_get_property(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CALL: {
            vm_result result = op_call(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CLOSURE: {
            vm_result result = op_closure(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_ARRAY: {
            vm_result result = op_build_array(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_OBJECT: {
            vm_result result = op_build_object(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_DEBUG_LOCATION: {
            vm_result result = op_set_debug_location(vm);
            if (result != VM_OK) return result;
            break;
        }
        
        case OP_CLEAR_DEBUG_LOCATION: {
            vm_result result = op_clear_debug_location(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_JUMP: {
            vm_result result = op_jump(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_JUMP_IF_FALSE: {
            vm_result result = op_jump_if_false(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LOOP: {
            vm_result result = op_loop(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POP_N: {
            vm_result result = op_pop_n(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_HALT: {
            vm_result result = op_halt(vm);
            return result;
        }

        // Missing opcodes that were causing test failures
        case OP_BITWISE_AND: {
            vm_result result = op_bitwise_and(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BITWISE_OR: {
            vm_result result = op_bitwise_or(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BITWISE_XOR: {
            vm_result result = op_bitwise_xor(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BITWISE_NOT: {
            vm_result result = op_bitwise_not(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LEFT_SHIFT: {
            vm_result result = op_left_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_RIGHT_SHIFT: {
            vm_result result = op_right_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_LOGICAL_RIGHT_SHIFT: {
            vm_result result = op_logical_right_shift(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_FLOOR_DIV: {
            vm_result result = op_floor_div(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_INCREMENT: {
            vm_result result = op_increment(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DECREMENT: {
            vm_result result = op_decrement(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_IN: {
            vm_result result = op_in(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_CALL_METHOD: {
            vm_result result = op_call_method(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_POP_N_PRESERVE_TOP: {
            vm_result result = op_pop_n_preserve_top(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_BUILD_RANGE: {
            vm_result result = op_build_range(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_IMPORT_MODULE: {
            vm_result result = op_import_module(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_EXPORT: {
            vm_result result = op_get_export(vm);
            if (result != VM_OK) return result;
            break;
        }

        default:
            printf("DEBUG: Unimplemented opcode in vm_run: %d\n", instruction);
            return VM_RUNTIME_ERROR;
        }
    }
}

// VM execution with setup - clears stack and sets up initial call frame
vm_result vm_execute(vm_t* vm, function_t* function) {
    if (!vm || !function)
        return VM_RUNTIME_ERROR;

    // Clear the stack at the start of each execution (important for REPL)
    vm->stack_top = vm->stack;

    // Set up initial call frame
    if (vm->frame_count >= vm->frame_capacity) {
        return VM_STACK_OVERFLOW;
    }

    call_frame* frame = &vm->frames[vm->frame_count++];
    closure_t* closure = closure_create(function); // Simple closure wrapper
    frame->closure = closure;
    frame->ip = function->bytecode;
    frame->slots = vm->stack_top;

    vm->bytecode = function->bytecode;
    vm->ip = function->bytecode;

    // Run the VM using the new core execution function
    vm_result result = vm_run(vm);
    
    // Clean up closure if execution completed normally
    if (result == VM_OK) {
        closure_destroy(closure);
    }
    
    return result;
}
vm_result vm_interpret(vm_t* vm, const char* source) {
    if (!vm || !source) return VM_RUNTIME_ERROR;
    
    // Tokenize
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    // Parse
    parser_t parser;
    parser_init(&parser, &lexer);
    ast_program* program = parse_program(&parser);
    
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return VM_COMPILE_ERROR;
    }
    
    // Generate code
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    
    if (codegen->had_error || !function) {
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return VM_COMPILE_ERROR;
    }
    
    // Execute
    vm_result result = vm_execute(vm, function);
    
    // Cleanup
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return result;
}