#include "unity.h"

// Forward declarations for test groups
void test_lexer_suite(void);
void test_parser_suite(void);
void test_vm_suite(void);
void test_conditionals_suite(void);
void test_while_loops_suite(void);
void test_infinite_loops_suite(void);
void test_builtins_suite(void);
void test_buffer_class_suite(void);
void test_integers_suite(void);
void test_arithmetic_suite(void);
void test_logical_suite(void);
void test_break_statements_suite(void);
void test_continue_statements_suite(void);
void test_nested_loops_suite(void);
void test_assignment_suite(void);
void test_string_suite(void);
void test_localdate_suite(void);
void test_localtime_suite(void);
void run_new_operators_tests(void);

void setUp(void) {
    // Setup code that runs before each test
}

void tearDown(void) {
    // Cleanup code that runs after each test
}

int main(void) {
    UNITY_BEGIN();

    // Run test suites
    test_lexer_suite();
    test_parser_suite();
    test_vm_suite();
    test_conditionals_suite();
    test_while_loops_suite();
    test_infinite_loops_suite();
    test_builtins_suite();
    test_buffer_class_suite();
    test_integers_suite();
    test_arithmetic_suite();
    test_logical_suite();
    test_break_statements_suite();
    test_continue_statements_suite();
    test_nested_loops_suite();
    test_assignment_suite();
    test_string_suite();
    test_localdate_suite();
    test_localtime_suite();
    run_new_operators_tests();

    return UNITY_END();
}
