#include "test_helpers.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include <sys/stat.h>

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

    vm_t* vm = vm_create();
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

    vm_t* vm = vm_create();
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

// === Module Testing Helper Implementations ===

#include "module.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Forward declaration of helper function from module.c
void copy_global_to_exports(const char* key, void* data, size_t size, void* context);

// Helper function to copy global variables to module namespace
void copy_global_to_namespace(const char* key, void* data, size_t size, void* context) {
    module_t* module = (module_t*)context;
    if (!module || !key || !data || size != sizeof(value_t)) {
        return;
    }
    
    value_t* value = (value_t*)data;
    do_set(module->namespace, key, value, sizeof(value_t));
}

// Helper to get the full path to a test module file
char* test_get_module_path(const char* module_name) {
    // Try multiple possible paths to find the test modules directory
    const char* possible_paths[] = {
        "../tests/modules/",           // One level up (cmake-build-debug-ninja)
        "tests/modules/",              // Same level (if running from source root)
        "../../tests/modules/",        // Two levels up (nested build dirs)
        "../../../tests/modules/",     // Three levels up (deeply nested)
        NULL
    };
    
    size_t base_len = strlen(module_name) + strlen(".slate") + 1;
    
    for (int i = 0; possible_paths[i] != NULL; i++) {
        size_t path_len = strlen(possible_paths[i]) + base_len;
        char* full_path = malloc(path_len);
        if (!full_path) continue;
        
        snprintf(full_path, path_len, "%s%s.slate", possible_paths[i], module_name);
        
        // Check if this path exists
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            return full_path; // Found it!
        }
        
        free(full_path);
    }
    
    // If nothing worked, fall back to the original approach
    return NULL;
}

// Create a temporary module from source code for testing
module_t* test_create_temp_module(const char* name, const char* source) {
    // Parse the source code
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        if (program) ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return NULL;
    }

    // Create VM for module compilation
    vm_t* vm = vm_create();
    vm->context = CTX_TEST;
    
    // Compile the program
    codegen_t* codegen = codegen_create(vm);
    function_t* init_function = codegen_compile(codegen, program);
    if (codegen->had_error || !init_function) {
        vm_destroy(vm);
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return NULL;
    }

    // Create the module (for testing, we don't need VM reference)
    module_t* module = module_create(name, "", vm);
    module->init_function = init_function;
    
    // Execute the initialization to populate globals
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, init_function);
        if (result == VM_OK) {
            // Copy VM globals to module namespace, then to exports
            do_foreach_property(vm->globals, copy_global_to_namespace, module);
            do_foreach_property(module->namespace, copy_global_to_exports, module);
            module->state = MODULE_LOADED;
        } else {
            module_destroy(module);
            module = NULL;
        }
    } else {
        // Error during execution
        module_destroy(module);  
        module = NULL;
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return module;
}

// Execute code with access to test modules (resolves modules from tests/modules/ directory)
value_t test_execute_with_imports(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        if (program) ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return make_null();
    }

    vm_t* vm = vm_create();
    vm->context = CTX_TEST;
    
    // Initialize module system
    module_system_init(vm);
    
    // Add tests/modules directory to module search paths
    // Try multiple possible paths to find the test modules directory
    const char* possible_search_paths[] = {
        "../tests/modules",           // One level up (cmake-build-debug-ninja)
        "tests/modules",              // Same level (if running from source root)
        "../../tests/modules",        // Two levels up (nested build dirs)
        "../../../tests/modules",     // Three levels up (deeply nested)
        NULL
    };
    
    for (int i = 0; possible_search_paths[i] != NULL; i++) {
        // Check if this search path exists by testing a known file
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/declarations.slate", possible_search_paths[i]);
        
        struct stat st;
        if (stat(test_path, &st) == 0 && S_ISREG(st.st_mode)) {
            module_add_search_path(vm, possible_search_paths[i]);
            break; // Found a valid path, stop searching
        }
    }
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    if (codegen->had_error || !function) {
        module_system_cleanup(vm);
        vm_destroy(vm);
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return make_null();
    }

    value_t return_value = make_null();
    
    // Execute with module support
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, function);
        if (result == VM_OK) {
            return_value = vm->result;
            return_value = vm_retain(return_value);
        }
    }

    module_system_cleanup(vm);
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return return_value;
}

// Test that an import statement fails with expected error
bool test_expect_import_error(const char* import_source, ErrorKind expected_error) {
    lexer_t lexer;
    lexer_init(&lexer, import_source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error) {
        // Parse error - check if this is the expected error type
        if (program) ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return false; // For now, focus on runtime import errors
    }
    
    if (!program) {
        lexer_cleanup(&lexer);
        return false;
    }

    vm_t* vm = vm_create();
    vm->context = CTX_TEST;
    
    // Initialize module system
    module_system_init(vm);
    
    // Add tests/modules directory to module search paths for error testing too
    // Try multiple possible paths to find the test modules directory
    const char* possible_search_paths2[] = {
        "../tests/modules",           // One level up (cmake-build-debug-ninja)
        "tests/modules",              // Same level (if running from source root)
        "../../tests/modules",        // Two levels up (nested build dirs)
        "../../../tests/modules",     // Three levels up (deeply nested)
        NULL
    };
    
    for (int i = 0; possible_search_paths2[i] != NULL; i++) {
        // Check if this search path exists by testing a known file
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/declarations.slate", possible_search_paths2[i]);
        
        struct stat st;
        if (stat(test_path, &st) == 0 && S_ISREG(st.st_mode)) {
            module_add_search_path(vm, possible_search_paths2[i]);
            break; // Found a valid path, stop searching
        }
    }
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    if (codegen->had_error) {
        module_system_cleanup(vm);
        vm_destroy(vm);
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return false; // Compile error, not import error
    }
    
    if (!function) {
        module_system_cleanup(vm);
        vm_destroy(vm);
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return false;
    }

    bool error_occurred = false;
    ErrorKind actual_error = ERR_NONE;
    
    // Execute and catch import errors
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, function);
        error_occurred = false;
    } else {
        error_occurred = true;
        actual_error = vm->error.kind;
    }

    module_system_cleanup(vm);
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return error_occurred && actual_error == expected_error;
}