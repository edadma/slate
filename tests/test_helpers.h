#pragma once

#include "unity.h"
#include "vm.h"
#include "value.h"
#include "module.h"

// Helper function to compile and execute a source string, returning the result value
// This version includes proper error handling with setjmp/longjmp for CTX_TEST context
// Returns make_null() on runtime errors (for compatibility with existing tests)
value_t test_execute_expression(const char* source);

// Helper function to execute source and expect a specific error kind
// Returns true if the expected error occurred, false otherwise
bool test_expect_error(const char* source, ErrorKind expected_error);

// === Module Testing Helpers ===

// Create a temporary module from source code for testing
// Returns NULL on parse/compile errors
module_t* test_create_temp_module(const char* name, const char* source);

// Execute code with access to test modules (resolves modules from tests/modules/ directory)
// Returns result value, or make_null() on errors
value_t test_execute_with_imports(const char* source);

// Test that an import statement fails with expected error
// Returns true if the expected error occurred, false otherwise
bool test_expect_import_error(const char* import_source, ErrorKind expected_error);

// Helper to get the full path to a test module file
// Caller must free the returned string
char* test_get_module_path(const char* module_name);