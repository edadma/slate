#pragma once

#include "unity.h"
#include "vm.h"
#include "value.h"

// Helper function to compile and execute a source string, returning the result value
// This version includes proper error handling with setjmp/longjmp for CTX_TEST context
// Returns make_null() on runtime errors (for compatibility with existing tests)
value_t test_execute_expression(const char* source);

// Helper function to execute source and expect a specific error kind
// Returns true if the expected error occurred, false otherwise
bool test_expect_error(const char* source, ErrorKind expected_error);