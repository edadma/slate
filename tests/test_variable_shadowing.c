#include <math.h>
#include <string.h>
#include "test_helpers.h"
#include "unity.h"
#include "vm.h"

// Use the standardized test helper from test_vm.c
extern value_t run_code(const char* source);

// Test basic for loop index variable shadowing
void test_for_loop_basic_shadowing(void) {
    value_t result;
    
    // Test: Variable 'i' accessible before, shadowed during, restored after for loop
    result = run_code(
        "var i = 'global_i' \n"
        "var before = i + ' before' \n"
        "var during = '' \n"
        "for var i = 1; i <= 3; i += 1 do during += i + ' ' \n"
        "var after = i + ' after' \n"
        "before + '|' + during + '|' + after"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global_i before|1 2 3 |global_i after", result.as.string);
    vm_release(result);
}

// Test for loop shadowing with initially undefined variable (null)
void test_for_loop_undefined_shadowing(void) {
    value_t result;
    
    // Test: Variable starts as null, shadowed by for loop, then restored to null
    result = run_code(
        "var x = null \n"
        "var original = x + ' original' \n"
        "var loop_values = '' \n"
        "for var x = 5; x <= 6; x += 1 do loop_values += x + ' ' \n"
        "var restored = x + ' original' \n"
        "original + '|' + loop_values + '|' + restored"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null original|5 6 |null original", result.as.string);
    vm_release(result);
}

// Test nested for loops with same variable name
void test_nested_for_loops_shadowing(void) {
    value_t result;
    
    // Test: Nested for loops both using 'i', with proper restoration
    result = run_code(
        "var i = 'outer_global' \n"
        "var trace = i + ' start|' \n"
        "for var i = 1; i <= 2; i += 1 do \n"
        "    trace += i + ' outer|' \n"
        "    for var i = 10; i <= 11; i += 1 do \n"
        "        trace += i + ' inner|' \n"
        "    trace += i + ' restored-outer|' \n"
        "trace += i + ' end' \n"
        "trace"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("outer_global start|1 outer|10 inner|11 inner|1 restored-outer|2 outer|10 inner|11 inner|2 restored-outer|outer_global end", result.as.string);
    vm_release(result);
}

// Test function parameter shadowing
void test_function_parameter_shadowing(void) {
    value_t result;
    
    // Test: Global variable shadowed by function parameter
    result = run_code(
        "var name = 'global' \n"
        "var before = name + ' before' \n"
        "def greet(name) = name + ' function' \n"
        "var during = greet('parameter') \n"
        "var after = name + ' after' \n"
        "before + '|' + during + '|' + after"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global before|parameter function|global after", result.as.string);
    vm_release(result);
}

// Test function local variable shadowing
void test_function_local_shadowing(void) {
    value_t result;
    
    // Test: Global variable shadowed by function local variable
    result = run_code(
        "var value = 'global' \n"
        "var trace = value + ' start|' \n"
        "def test() = \n"
        "    var value = 'local' \n"
        "    value + ' function' \n"
        "trace += test() + '|' \n"
        "trace += value + ' end' \n"
        "trace"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global start|local function|global end", result.as.string);
    vm_release(result);
}

// Test if block variable shadowing
void test_if_block_shadowing(void) {
    value_t result;
    
    // Test: Variable shadowed in if block, then restored
    result = run_code(
        "var status = 'global' \n"
        "var before = status + ' before' \n"
        "var block_result = '' \n"
        "if true then \n"
        "    var status = 'block' \n"
        "    block_result = status + ' inside' \n"
        "var after = status + ' after' \n"
        "before + '|' + block_result + '|' + after"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global before|block inside|global after", result.as.string);
    vm_release(result);
}

// Test while block variable shadowing
void test_while_block_shadowing(void) {
    value_t result;
    
    // Test: Variable shadowed in while block, then restored
    result = run_code(
        "var counter = 'global' \n"
        "var before = counter + ' before' \n"
        "var iterations = 0 \n"
        "var loop_values = '' \n"
        "while iterations < 2 do \n"
        "    var counter = iterations + 1 \n"
        "    loop_values += counter + ' ' \n"
        "    iterations += 1 \n"
        "var after = counter + ' after' \n"
        "before + '|' + loop_values + '|' + after"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global before|1 2 |global after", result.as.string);
    vm_release(result);
}

// Test for loop inside function (function -> loop shadowing)
void test_for_loop_in_function_shadowing(void) {
    value_t result;
    
    // Test: Global shadowed by function, then by for loop within function
    result = run_code(
        "var x = 'global' \n"
        "var trace = x + ' start|' \n"
        "def loopTest() = \n"
        "    var x = 'function' \n"
        "    var func_trace = x + ' func|' \n"
        "    for var x = 1; x <= 2; x += 1 do \n"
        "        func_trace += x + ' loop|' \n"
        "    func_trace += x + ' func-end' \n"
        "    func_trace \n"
        "trace += loopTest() + '|' \n"
        "trace += x + ' global-end' \n"
        "trace"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global start|function func|1 loop|2 loop|function func-end|global global-end", result.as.string);
    vm_release(result);
}

// Test triple nested shadowing: function -> block -> loop
void test_triple_nested_shadowing(void) {
    value_t result;
    
    // Test: Variable 'data' shadowed at function, block, and loop levels
    result = run_code(
        "var data = 'global' \n"
        "var trace = data + ' start|' \n"
        "def complexTest() = \n"
        "    var data = 'function' \n"
        "    trace += data + ' func|' \n"
        "    if true then \n"
        "        var data = 'block' \n"
        "        trace += data + ' block|' \n"
        "        for var data = 1; data <= 2; data += 1 do \n"
        "            trace += data + ' loop|' \n"
        "        trace += data + ' block-restored|' \n"
        "    trace += data + ' func-restored' \n"
        "    data \n"
        "var func_result = complexTest() \n"
        "trace += '|' + func_result + '|' + data + ' global-end' \n"
        "trace"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("global start|function func|block block|1 loop|2 loop|block block-restored|function func-restored|function|global global-end", result.as.string);
    vm_release(result);
}

// Test same variable name shadowed at every possible level
void test_comprehensive_multilevel_shadowing(void) {
    value_t result;
    
    // Test: Variable 'v' used at global, function, block, and loop scopes
    result = run_code(
        "var v = 'L0' \n"                    // Level 0: Global
        "var trace = v + '|' \n"
        "def test(v) = \n"                   // Level 1: Function parameter
        "    trace += v + '|' \n"
        "    if true then \n"
        "        var v = 'L2' \n"            // Level 2: Block variable
        "        trace += v + '|' \n"
        "        for var v = 1; v <= 1; v += 1 do \n"  // Level 3: Loop variable
        "            trace += v + '|' \n"
        "        trace += v + '|' \n"        // Back to Level 2
        "    trace += v + '|' \n"            // Back to Level 1
        "    'done' \n"
        "test('L1') \n"
        "trace += v \n"                      // Back to Level 0
        "trace"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("L0|L1|L2|1|L2|L1|L0", result.as.string);
    vm_release(result);
}

// Variable shadowing test suite
void test_variable_shadowing_suite(void) {
    RUN_TEST(test_for_loop_basic_shadowing);
    RUN_TEST(test_for_loop_undefined_shadowing);
    RUN_TEST(test_nested_for_loops_shadowing);
    RUN_TEST(test_function_parameter_shadowing);
    RUN_TEST(test_function_local_shadowing);
    RUN_TEST(test_if_block_shadowing);
    RUN_TEST(test_while_block_shadowing);
    RUN_TEST(test_for_loop_in_function_shadowing);
    RUN_TEST(test_triple_nested_shadowing);
    RUN_TEST(test_comprehensive_multilevel_shadowing);
}