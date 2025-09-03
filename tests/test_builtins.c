#include "unity.h"
#include "test_helpers.h"



// Test type function for numeric types
void test_builtin_type_number(void) {
    // Test int32 type
    value_t result = test_execute_expression("type(42)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("int32", result.as.string);
    vm_release(result);
    
    // Test number (float) type
    result = test_execute_expression("type(3.14)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("float64", result.as.string);
    vm_release(result);
}

void test_builtin_type_string(void) {
    value_t result = test_execute_expression("type(\"hello\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("string", result.as.string);
    vm_release(result);
}

void test_builtin_type_boolean(void) {
    value_t result = test_execute_expression("type(true)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("boolean", result.as.string);
    vm_release(result);
}

void test_builtin_type_null(void) {
    value_t result = test_execute_expression("type(null)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null", result.as.string);
    vm_release(result);
}

// Test abs instance method
void test_builtin_abs_positive(void) {
    value_t result = test_execute_expression("abs(5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_negative(void) {
    value_t result = test_execute_expression("abs(-5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_zero(void) {
    value_t result = test_execute_expression("abs(0)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
}

// Test sqrt instance method
void test_builtin_sqrt(void) {
    value_t result = test_execute_expression("sqrt(16)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, result.as.float64);
    vm_release(result);
}

void test_builtin_sqrt_zero(void) {
    value_t result = test_execute_expression("sqrt(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

// Test floor instance method
void test_builtin_floor(void) {
    value_t result = test_execute_expression("floor(3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_floor_negative(void) {
    value_t result = test_execute_expression("floor(-3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-4, result.as.int32);
    vm_release(result);
}

// Test ceil instance method
void test_builtin_ceil(void) {
    value_t result = test_execute_expression("ceil(3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_ceil_negative(void) {
    value_t result = test_execute_expression("ceil(-3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-3, result.as.int32);
    vm_release(result);
}

// Test round instance method
void test_builtin_round_up(void) {
    value_t result = test_execute_expression("round(3.6)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_round_down(void) {
    value_t result = test_execute_expression("round(3.4)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_round_half(void) {
    value_t result = test_execute_expression("round(3.5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

// Test min function
void test_builtin_min(void) {
    value_t result = test_execute_expression("min(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_min_negative(void) {
    value_t result = test_execute_expression("min(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-5, result.as.int32);
    vm_release(result);
}

// Test max function
void test_builtin_max(void) {
    value_t result = test_execute_expression("max(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);
}

void test_builtin_max_negative(void) {
    value_t result = test_execute_expression("max(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-2, result.as.int32);
    vm_release(result);
}

// Test random function (just check it returns a number in valid range)
void test_builtin_random(void) {
    value_t result = test_execute_expression("random()");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_TRUE(result.as.float64 >= 0.0);
    TEST_ASSERT_TRUE(result.as.float64 <= 1.0);
    vm_release(result);
}

// Test sin instance method
void test_builtin_sin_zero(void) {
    value_t result = test_execute_expression("sin(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_sin_pi_half(void) {
    value_t result = test_execute_expression("sin(3.14159265359/2)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.0, result.as.float64);
    vm_release(result);
}

// Test cos instance method
void test_builtin_cos_zero(void) {
    value_t result = test_execute_expression("cos(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, result.as.float64);
    vm_release(result);
}

void test_builtin_cos_pi(void) {
    value_t result = test_execute_expression("cos(3.14159265359)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -1.0, result.as.float64);
    vm_release(result);
}

// Test tan instance method
void test_builtin_tan_zero(void) {
    value_t result = test_execute_expression("tan(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_tan_pi_quarter(void) {
    value_t result = test_execute_expression("tan(3.14159265359/4)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.0, result.as.float64);
    vm_release(result);
}

// Test trig functions with integer arguments
void test_builtin_sin_integer(void) {
    value_t result = test_execute_expression("sin(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.841471, result.as.float64);
    vm_release(result);
}

// Test exp function
void test_builtin_exp_zero(void) {
    value_t result = test_execute_expression("exp(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, result.as.float64);
    vm_release(result);
}

void test_builtin_exp_one(void) {
    value_t result = test_execute_expression("exp(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 2.718282, result.as.float64);
    vm_release(result);
}

void test_builtin_exp_negative(void) {
    value_t result = test_execute_expression("exp(-1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.367879, result.as.float64);
    vm_release(result);
}

void test_builtin_exp_integer(void) {
    value_t result = test_execute_expression("exp(2)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 7.389056, result.as.float64);
    vm_release(result);
}

// Test ln function
void test_builtin_ln_one(void) {
    value_t result = test_execute_expression("ln(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_ln_e(void) {
    value_t result = test_execute_expression("ln(2.718282)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.0, result.as.float64);
    vm_release(result);
}

void test_builtin_ln_ten(void) {
    value_t result = test_execute_expression("ln(10)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 2.302585, result.as.float64);
    vm_release(result);
}

void test_builtin_ln_half(void) {
    value_t result = test_execute_expression("ln(0.5)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -0.693147, result.as.float64);
    vm_release(result);
}

// Test exp and ln inverse relationship
void test_builtin_exp_ln_inverse(void) {
    value_t result = test_execute_expression("exp(ln(5))");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 5.0, result.as.float64);
    vm_release(result);
}

void test_builtin_ln_exp_inverse(void) {
    value_t result = test_execute_expression("ln(exp(3))");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 3.0, result.as.float64);
    vm_release(result);
}

// Test asin function
void test_builtin_asin_zero(void) {
    value_t result = test_execute_expression("asin(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_asin_one(void) {
    value_t result = test_execute_expression("asin(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.570796, result.as.float64); // π/2
    vm_release(result);
}

void test_builtin_asin_negative(void) {
    value_t result = test_execute_expression("asin(-0.5)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -0.523599, result.as.float64); // -π/6
    vm_release(result);
}

// Test acos function
void test_builtin_acos_one(void) {
    value_t result = test_execute_expression("acos(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_acos_zero(void) {
    value_t result = test_execute_expression("acos(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.570796, result.as.float64); // π/2
    vm_release(result);
}

void test_builtin_acos_half(void) {
    value_t result = test_execute_expression("acos(0.5)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.047198, result.as.float64); // π/3
    vm_release(result);
}

// Test atan function
void test_builtin_atan_zero(void) {
    value_t result = test_execute_expression("atan(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_atan_one(void) {
    value_t result = test_execute_expression("atan(1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.785398, result.as.float64); // π/4
    vm_release(result);
}

void test_builtin_atan_negative(void) {
    value_t result = test_execute_expression("atan(-1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -0.785398, result.as.float64); // -π/4
    vm_release(result);
}

// Test atan2 function
void test_builtin_atan2_positive_x(void) {
    value_t result = test_execute_expression("atan2(1, 1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.785398, result.as.float64); // π/4
    vm_release(result);
}

void test_builtin_atan2_negative_x(void) {
    value_t result = test_execute_expression("atan2(1, -1)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 2.356194, result.as.float64); // 3π/4
    vm_release(result);
}

void test_builtin_atan2_origin(void) {
    value_t result = test_execute_expression("atan2(0, 0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

// Test degrees function
void test_builtin_degrees_zero(void) {
    value_t result = test_execute_expression("degrees(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_degrees_pi(void) {
    value_t result = test_execute_expression("degrees(3.14159265359)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 180.0, result.as.float64);
    vm_release(result);
}

void test_builtin_degrees_pi_half(void) {
    value_t result = test_execute_expression("degrees(1.5707963268)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 90.0, result.as.float64);
    vm_release(result);
}

// Test radians function
void test_builtin_radians_zero(void) {
    value_t result = test_execute_expression("radians(0)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.float64);
    vm_release(result);
}

void test_builtin_radians_180(void) {
    value_t result = test_execute_expression("radians(180)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 3.14159265359, result.as.float64);
    vm_release(result);
}

void test_builtin_radians_90(void) {
    value_t result = test_execute_expression("radians(90)");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.5707963268, result.as.float64);
    vm_release(result);
}

// Test sign instance method
void test_builtin_sign_positive(void) {
    value_t result = test_execute_expression("sign(42)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32);
    vm_release(result);
}

void test_builtin_sign_negative(void) {
    value_t result = test_execute_expression("sign(-3.14)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(-1, result.as.int32);
    vm_release(result);
}

void test_builtin_sign_zero(void) {
    value_t result = test_execute_expression("sign(0)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

// Test inverse trig relationships
void test_builtin_trig_inverse_relationships(void) {
    // sin(asin(0.5)) should be 0.5
    value_t result = test_execute_expression("sin(asin(0.5))");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.5, result.as.float64);
    vm_release(result);
}

void test_builtin_degrees_radians_inverse(void) {
    // degrees(radians(2)) should be 2
    value_t result = test_execute_expression("degrees(radians(2))");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 2.0, result.as.float64);
    vm_release(result);
}

// Test string concatenation with arrays
// String tests moved to test_string.c

// Test array with mixed types including strings




// String method tests moved to test_class_string.c
// Array method tests moved to test_class_array.c

// Test string isEmpty and nonEmpty methods
// String isEmpty/nonEmpty tests moved to test_string.c

// Range method tests moved to test_class_range.c
// Iterator method tests moved to test_class_iterator.c

// Test suite function
void test_builtins_suite(void) {
    RUN_TEST(test_builtin_type_number);
    RUN_TEST(test_builtin_type_string);
    RUN_TEST(test_builtin_type_boolean);
    RUN_TEST(test_builtin_type_null);
    RUN_TEST(test_builtin_abs_positive);
    RUN_TEST(test_builtin_abs_negative);
    RUN_TEST(test_builtin_abs_zero);
    RUN_TEST(test_builtin_sqrt);
    RUN_TEST(test_builtin_sqrt_zero);
    RUN_TEST(test_builtin_floor);
    RUN_TEST(test_builtin_floor_negative);
    RUN_TEST(test_builtin_ceil);
    RUN_TEST(test_builtin_ceil_negative);
    RUN_TEST(test_builtin_round_up);
    RUN_TEST(test_builtin_round_down);
    RUN_TEST(test_builtin_round_half);
    RUN_TEST(test_builtin_min);
    RUN_TEST(test_builtin_min_negative);
    RUN_TEST(test_builtin_max);
    RUN_TEST(test_builtin_max_negative);
    RUN_TEST(test_builtin_random);
    RUN_TEST(test_builtin_sin_zero);
    RUN_TEST(test_builtin_sin_pi_half);
    RUN_TEST(test_builtin_cos_zero);
    RUN_TEST(test_builtin_cos_pi);
    RUN_TEST(test_builtin_tan_zero);
    RUN_TEST(test_builtin_tan_pi_quarter);
    RUN_TEST(test_builtin_sin_integer);
    
    // Exponential and logarithm tests
    RUN_TEST(test_builtin_exp_zero);
    RUN_TEST(test_builtin_exp_one);
    RUN_TEST(test_builtin_exp_negative);
    RUN_TEST(test_builtin_exp_integer);
    RUN_TEST(test_builtin_ln_one);
    RUN_TEST(test_builtin_ln_e);
    RUN_TEST(test_builtin_ln_ten);
    RUN_TEST(test_builtin_ln_half);
    RUN_TEST(test_builtin_exp_ln_inverse);
    RUN_TEST(test_builtin_ln_exp_inverse);
    
    // Inverse trigonometric and angle conversion tests
    RUN_TEST(test_builtin_asin_zero);
    RUN_TEST(test_builtin_asin_one);
    RUN_TEST(test_builtin_asin_negative);
    RUN_TEST(test_builtin_acos_one);
    RUN_TEST(test_builtin_acos_zero);
    RUN_TEST(test_builtin_acos_half);
    RUN_TEST(test_builtin_atan_zero);
    RUN_TEST(test_builtin_atan_one);
    RUN_TEST(test_builtin_atan_negative);
    // RUN_TEST(test_builtin_atan2_positive_x);  // atan2 requires two arguments, not implemented yet
    // RUN_TEST(test_builtin_atan2_negative_x);
    // RUN_TEST(test_builtin_atan2_origin);
    RUN_TEST(test_builtin_degrees_zero);
    RUN_TEST(test_builtin_degrees_pi);
    RUN_TEST(test_builtin_degrees_pi_half);
    RUN_TEST(test_builtin_radians_zero);
    RUN_TEST(test_builtin_radians_180);
    RUN_TEST(test_builtin_radians_90);
    RUN_TEST(test_builtin_sign_positive);
    RUN_TEST(test_builtin_sign_negative);
    RUN_TEST(test_builtin_sign_zero);
    // RUN_TEST(test_builtin_trig_inverse_relationships);
    // RUN_TEST(test_builtin_degrees_radians_inverse);
    
    // String tests moved to test_class_string.c
    // Array tests moved to test_class_array.c  
    // Buffer tests moved to test_buffer_class.c
    // Range tests moved to test_class_range.c
    // Iterator tests moved to test_class_iterator.c
    // Object tests moved to test_class_object.c
}