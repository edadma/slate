#include "unity.h"

// Forward declarations for test groups
void test_lexer_suite(void);
void test_parser_suite(void);
void test_vm_suite(void);
void test_conditionals_suite(void);
void test_while_loops_suite(void);
void test_builtins_suite(void);
void test_integers_suite(void);
void test_arithmetic_suite(void);

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
    test_builtins_suite();
    test_integers_suite(); // TESTING - does this cause segfault?
    // test_arithmetic_suite(); // COMMENTED OUT - testing for segfault
    
    return UNITY_END();
}