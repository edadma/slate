#include "test_helpers.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"

// Helper function to compile and execute a source string, returning the result value
// This version includes proper error handling with setjmp/longjmp for CTX_TEST context
// Returns make_null() on runtime errors (for compatibility with existing tests)
value_t test_execute_expression(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_NOT_NULL(program);

    slate_vm* vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    TEST_ASSERT_FALSE(codegen->had_error);
    TEST_ASSERT_NOT_NULL(function);

    value_t return_value = make_null();
    
    // Use setjmp to catch errors from slate_runtime_error
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, function);
        if (result == VM_OK) {
            return_value = vm->result;
            // Retain strings and other reference-counted types to survive cleanup
            return_value = vm_retain(return_value);
        }
    } else {
        // Error occurred - check vm->error for details if needed
        // For now, return null as before (tests expect null on error)
        return_value = make_null();
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return return_value;
}

// Helper function to execute source and expect a specific error kind
// Returns true if the expected error occurred, false otherwise
bool test_expect_error(const char* source, ErrorKind expected_error) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error) {
        // Parse error - not a runtime error we can catch
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return false;
    }
    
    TEST_ASSERT_NOT_NULL(program);

    slate_vm* vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    if (codegen->had_error) {
        // Compile error - not a runtime error we can catch
        vm_destroy(vm);
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return false;
    }
    
    TEST_ASSERT_NOT_NULL(function);

    bool error_occurred = false;
    ErrorKind actual_error = ERR_NONE;
    
    // Use setjmp to catch errors from slate_runtime_error
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, function);
        // If we get here, no error occurred
        error_occurred = false;
    } else {
        // Error occurred - check if it matches what we expected
        error_occurred = true;
        actual_error = vm->error.kind;
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return error_occurred && actual_error == expected_error;
}