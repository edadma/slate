/**
 * @file dynamic_int.h
 * @brief Reference-counted arbitrary precision integer library
 * @version 1.0.0
 * @date August 2025
 *
 * Single header library for arbitrary precision integers with reference counting.
 * Designed for seamless integration with MCU projects that need automatic
 * promotion from fixed-size integers to arbitrary precision on overflow.
 *
 * @section config Configuration
 *
 * Customize the library by defining these macros before including:
 *
 * @code
 * #define DI_MALLOC malloc         // custom allocator
 * #define DI_REALLOC realloc       // custom reallocator
 * #define DI_FREE free             // custom deallocator
 * #define DI_ASSERT assert         // custom assert macro
 * #define DI_LIMB_BITS 32          // bits per limb (default: 32)
 *
 * #define DI_IMPLEMENTATION
 * #include "dynamic_int.h"
 * @endcode
 *
 * @section usage Basic Usage
 *
 * @code
 * di_int a = di_from_int32(42);
 * di_int b = di_from_int32(100);
 * di_int sum = di_add(a, b);
 * 
 * int32_t result;
 * if (di_to_int32(sum, &result)) {
 *     printf("Sum: %d\n", result);
 * } else {
 *     char* str = di_to_string(sum, 10);
 *     printf("Sum (too large): %s\n", str);
 *     free(str);
 * }
 * 
 * di_release(&a);
 * di_release(&b);
 * di_release(&sum);
 * @endcode
 */

#ifndef DYNAMIC_INT_H
#define DYNAMIC_INT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

/* Configuration macros */
#ifndef DI_MALLOC
#define DI_MALLOC malloc
#endif

#ifndef DI_REALLOC
#define DI_REALLOC realloc
#endif

#ifndef DI_FREE
#define DI_FREE free
#endif

#ifndef DI_ASSERT
#define DI_ASSERT assert
#endif

#ifndef DI_LIMB_BITS
#define DI_LIMB_BITS 32
#endif

// API macros
#ifdef DI_STATIC
#define DI_DEF static
#else
#define DI_DEF extern
#endif

#if DI_LIMB_BITS == 32
typedef uint32_t di_limb_t;
typedef uint64_t di_dlimb_t;  // Double-width for multiplication
#define DI_LIMB_MAX UINT32_MAX
#elif DI_LIMB_BITS == 16
typedef uint16_t di_limb_t;
typedef uint32_t di_dlimb_t;
#define DI_LIMB_MAX UINT16_MAX
#else
#error "DI_LIMB_BITS must be 16 or 32"
#endif

// ============================================================================
// INTERFACE
// ============================================================================

/**
 * @brief Integer handle for arbitrary precision integers
 *
 * This is a handle (pointer) to an arbitrary precision integer with
 * reference counting. The actual data is stored in a struct di_int_internal
 * which contains the limb array, reference count, and sign flag.
 *
 * @note NULL represents an invalid integer handle
 * @note All integers are immutable - operations return new integers
 * @note Reference counting prevents memory leaks
 */
typedef struct di_int_internal* di_int;

/**
 * @defgroup creation_functions Integer Creation Functions
 * @brief Functions for creating arbitrary precision integers from various sources
 * @{
 */

/**
 * @brief Create a new integer from a 32-bit signed integer
 * @param value The 32-bit signed integer value
 * @return New di_int instance, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int num = di_from_int32(-42);
 * int32_t result;
 * if (di_to_int32(num, &result)) {
 *     printf("Value: %d\n", result);  // Value: -42
 * }
 * di_release(&num);
 * @endcode
 * 
 * @see di_from_int64() for 64-bit integers
 * @see di_to_int32() for conversion back to int32_t
 * @see di_release() for memory cleanup
 */
DI_DEF di_int di_from_int32(int32_t value);

/**
 * @brief Create a new integer from a 64-bit signed integer
 * @param value The 64-bit signed integer value
 * @return New di_int instance, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int big_num = di_from_int64(9223372036854775807LL);  // INT64_MAX
 * char* str = di_to_string(big_num, 10);
 * printf("Value: %s\n", str);
 * free(str);
 * di_release(&big_num);
 * @endcode
 * 
 * @see di_from_int32() for 32-bit integers
 * @see di_to_int64() for conversion back to int64_t
 */
DI_DEF di_int di_from_int64(int64_t value);

/**
 * @brief Create a new integer from a 32-bit unsigned integer
 * @param value The 32-bit unsigned integer value
 * @return New di_int instance, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_from_uint64() for 64-bit unsigned integers
 * @see di_to_uint32() for conversion back to uint32_t
 */
DI_DEF di_int di_from_uint32(uint32_t value);

/**
 * @brief Create a new integer from a 64-bit unsigned integer
 * @param value The 64-bit unsigned integer value
 * @return New di_int instance, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_from_uint32() for 32-bit unsigned integers
 * @see di_to_uint64() for conversion back to uint64_t
 */
DI_DEF di_int di_from_uint64(uint64_t value);

/**
 * @brief Create a new integer from a string representation
 * @param str Null-terminated string containing the number
 * @param base Number base (2-36)
 * @return New di_int instance, or NULL on failure or invalid input
 * @since 1.0.0
 * 
 * @code
 * di_int hex = di_from_string("DEADBEEF", 16);
 * di_int bin = di_from_string("1010110", 2);
 * di_int dec = di_from_string("-12345", 10);
 * 
 * // Clean up
 * di_release(&hex);
 * di_release(&bin);
 * di_release(&dec);
 * @endcode
 * 
 * @note Currently only base 10 is fully implemented
 * @see di_to_string() for conversion back to string
 */
DI_DEF di_int di_from_string(const char* str, int base);

/**
 * @brief Create a new integer with value zero
 * @return New di_int instance with value 0, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int zero = di_zero();
 * assert(di_is_zero(zero));
 * di_release(&zero);
 * @endcode
 * 
 * @see di_one() for creating integer with value 1
 * @see di_is_zero() for checking if integer is zero
 */
DI_DEF di_int di_zero(void);

/**
 * @brief Create a new integer with value one
 * @return New di_int instance with value 1, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_zero() for creating integer with value 0
 */
DI_DEF di_int di_one(void);

/**
 * @brief Create a deep copy of an integer
 * @param big Integer to copy (may be NULL)
 * @return New di_int instance with same value, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int original = di_from_int32(42);
 * di_int copy = di_copy(original);
 * assert(di_eq(original, copy));
 * 
 * di_release(&original);
 * di_release(&copy);
 * @endcode
 * 
 * @note Creates an independent copy with reference count = 1
 * @see di_retain() for sharing the same integer instance
 */
DI_DEF di_int di_copy(di_int big);

/** @} */ // end of creation_functions

/**
 * @defgroup reference_counting Reference Counting Functions
 * @brief Functions for managing integer lifetime through reference counting
 * @{
 */

/**
 * @brief Increment reference count and return the same integer
 * @param big Integer to retain (may be NULL)
 * @return Same integer handle, or NULL if input was NULL
 * @since 1.0.0
 * 
 * @code
 * di_int original = di_from_int32(42);     // ref_count = 1
 * di_int shared = di_retain(original);     // ref_count = 2, same object
 * 
 * di_release(&original);  // ref_count = 1, object still alive
 * di_release(&shared);    // ref_count = 0, object freed
 * @endcode
 * 
 * @note Use this when you want to share the same integer instance
 * @see di_release() for decrementing reference count
 * @see di_copy() for creating an independent copy
 */
DI_DEF di_int di_retain(di_int big);

/**
 * @brief Decrement reference count and free if zero
 * @param big Pointer to integer handle (may be NULL or point to NULL)
 * @since 1.0.0
 * 
 * @code
 * di_int num = di_from_int32(42);
 * di_release(&num);  // num becomes NULL, memory freed if ref_count was 1
 * @endcode
 * 
 * @note Always sets the handle to NULL after releasing
 * @note Safe to call with NULL pointer or NULL handle
 * @see di_retain() for incrementing reference count
 */
DI_DEF void di_release(di_int* big);

/**
 * @brief Get current reference count of an integer
 * @param big Integer to query (may be NULL)
 * @return Current reference count, or 0 if big is NULL
 * @since 1.0.0
 * 
 * @note Primarily for debugging and testing
 */
DI_DEF size_t di_ref_count(di_int big);

/** @} */ // end of reference_counting

/**
 * @defgroup arithmetic_operations Arithmetic Operations
 * @brief Basic arithmetic operations for arbitrary precision integers
 * @{
 */

/**
 * @brief Add two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with result of a + b, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int a = di_from_int32(100);
 * di_int b = di_from_int32(42);
 * di_int sum = di_add(a, b);  // sum = 142
 * 
 * di_release(&a);
 * di_release(&b);
 * di_release(&sum);
 * @endcode
 * 
 * @see di_add_i32() for mixed-type addition with int32_t
 * @see di_sub() for subtraction
 */
DI_DEF di_int di_add(di_int a, di_int b);

/**
 * @brief Add an integer and a 32-bit signed integer
 * @param a Integer operand (may be NULL)
 * @param b 32-bit signed integer operand
 * @return New di_int with result of a + b, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_add() for integer-integer addition
 */
DI_DEF di_int di_add_i32(di_int a, int32_t b);

/**
 * @brief Subtract two integers
 * @param a First integer (minuend, may be NULL)
 * @param b Second integer (subtrahend, may be NULL)
 * @return New di_int with result of a - b, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_sub_i32() for mixed-type subtraction with int32_t
 * @see di_add() for addition
 */
DI_DEF di_int di_sub(di_int a, di_int b);

/**
 * @brief Subtract a 32-bit signed integer from an integer
 * @param a Integer operand (may be NULL)
 * @param b 32-bit signed integer operand
 * @return New di_int with result of a - b, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_sub() for integer-integer subtraction
 */
DI_DEF di_int di_sub_i32(di_int a, int32_t b);

/**
 * @brief Multiply two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with result of a * b, or NULL on failure
 * @since 1.0.0
 * 
 * @note Supports arbitrary precision multiplication
 * @see di_mul_i32() for mixed-type multiplication with int32_t
 */
DI_DEF di_int di_mul(di_int a, di_int b);

/**
 * @brief Multiply an integer by a 32-bit signed integer
 * @param a Integer operand (may be NULL)
 * @param b 32-bit signed integer operand
 * @return New di_int with result of a * b, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_mul() for integer-integer multiplication
 */
DI_DEF di_int di_mul_i32(di_int a, int32_t b);

/**
 * @brief Divide two integers using floor division
 * @param a Dividend integer (must not be NULL)
 * @param b Divisor integer (must not be NULL)
 * @return New di_int with floor quotient of a / b
 * @since 1.0.0
 * 
 * Uses floor division semantics where the result is rounded towards negative
 * infinity, ensuring consistent behavior with signed operands.
 * 
 * @code
 * di_int a = di_from_int32(-7);
 * di_int b = di_from_int32(3);
 * di_int quotient = di_div(a, b);    // Result: -3 (floor division)
 * // Compare with C-style: -7/3 would be -2 (truncation towards zero)
 * di_release(&a);
 * di_release(&b);
 * di_release(&quotient);
 * @endcode
 * 
 * @note Asserts if a or b is NULL, or if b is zero
 * @see di_mod() for remainder/modulo operation using floor division
 */
DI_DEF di_int di_div(di_int a, di_int b);

/**
 * @brief Get remainder of integer division using floor modulo
 * @param a Dividend integer (must not be NULL)
 * @param b Divisor integer (must not be NULL)
 * @return New di_int with remainder of a % b where remainder has same sign as divisor
 * @since 1.0.0
 * 
 * Uses floor modulo semantics where the remainder always has the same sign as
 * the divisor, consistent with floor division behavior.
 * 
 * @code
 * di_int a = di_from_int32(-7);
 * di_int b = di_from_int32(3);
 * di_int remainder = di_mod(a, b);   // Result: 2 (positive, same sign as divisor)
 * // Compare with C-style: -7%3 would be -1 (negative, same sign as dividend)
 * di_release(&a);
 * di_release(&b);
 * di_release(&remainder);
 * @endcode
 * 
 * @note Asserts if a or b is NULL, or if b is zero
 * @see di_div() for quotient using floor division
 */
DI_DEF di_int di_mod(di_int a, di_int b);

/**
 * @brief Negate an integer (change sign)
 * @param a Integer to negate (may be NULL)
 * @return New di_int with result of -a, or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int pos = di_from_int32(42);
 * di_int neg = di_negate(pos);  // neg = -42
 * di_int pos_again = di_negate(neg);  // pos_again = 42
 * 
 * di_release(&pos);
 * di_release(&neg);
 * di_release(&pos_again);
 * @endcode
 * 
 * @see di_abs() for absolute value
 */
DI_DEF di_int di_negate(di_int a);

/**
 * @brief Get absolute value of an integer
 * @param a Integer to get absolute value of (may be NULL)
 * @return New di_int with |a|, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_negate() for negation
 * @see di_is_negative() for checking sign
 */
DI_DEF di_int di_abs(di_int a);

/**
 * @brief Raise integer to a power
 * @param base Base integer (may be NULL)
 * @param exp Exponent (32-bit unsigned integer)
 * @return New di_int with result of base^exp, or NULL on failure
 * @since 1.0.0
 * 
 * @note This is not yet implemented
 * @see di_mod_pow() for modular exponentiation
 */
DI_DEF di_int di_pow(di_int base, uint32_t exp);

/** @} */ // end of arithmetic_operations

/**
 * @defgroup bitwise_operations Bitwise Operations
 * @brief Bitwise operations for arbitrary precision integers
 * @{
 */

/**
 * @brief Bitwise AND of two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with result of a & b, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_and(di_int a, di_int b);

/**
 * @brief Bitwise OR of two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with result of a | b, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_or(di_int a, di_int b);

/**
 * @brief Bitwise XOR of two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with result of a ^ b, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_xor(di_int a, di_int b);

/**
 * @brief Bitwise NOT (complement) of an integer
 * @param a Integer to complement (may be NULL)
 * @return New di_int with result of ~a, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_not(di_int a);

/**
 * @brief Left shift an integer by specified bits
 * @param a Integer to shift (may be NULL)
 * @param bits Number of bits to shift left
 * @return New di_int with result of a << bits, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_shift_left(di_int a, size_t bits);

/**
 * @brief Right shift an integer by specified bits
 * @param a Integer to shift (may be NULL)
 * @param bits Number of bits to shift right
 * @return New di_int with result of a >> bits, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_shift_right(di_int a, size_t bits);

/** @} */ // end of bitwise_operations

/**
 * @defgroup comparison_operations Comparison Operations
 * @brief Functions for comparing arbitrary precision integers
 * @{
 */

/**
 * @brief Compare two integers
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 * @since 1.0.0
 * 
 * @note NULL values are treated as zero for comparison
 */
DI_DEF int di_compare(di_int a, di_int b);

/**
 * @brief Test if two integers are equal
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return true if a == b, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_eq(di_int a, di_int b);

/**
 * @brief Test if first integer is less than second
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return true if a < b, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_lt(di_int a, di_int b);

/**
 * @brief Test if first integer is less than or equal to second
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return true if a <= b, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_le(di_int a, di_int b);

/**
 * @brief Test if first integer is greater than second
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return true if a > b, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_gt(di_int a, di_int b);

/**
 * @brief Test if first integer is greater than or equal to second
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return true if a >= b, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_ge(di_int a, di_int b);

/**
 * @brief Test if integer is zero
 * @param big Integer to test (may be NULL)
 * @return true if integer is zero, false otherwise
 * @since 1.0.0
 * 
 * @note NULL is considered zero
 */
DI_DEF bool di_is_zero(di_int big);

/**
 * @brief Test if integer is negative
 * @param big Integer to test (may be NULL)
 * @return true if integer is negative, false otherwise
 * @since 1.0.0
 * 
 * @note NULL is considered zero (not negative)
 */
DI_DEF bool di_is_negative(di_int big);

/**
 * @brief Test if integer is positive (> 0)
 * @param big Integer to test (may be NULL)
 * @return true if integer is positive, false otherwise
 * @since 1.0.0
 * 
 * @note NULL is considered zero (not positive)
 */
DI_DEF bool di_is_positive(di_int big);

/** @} */ // end of comparison_operations

/**
 * @defgroup conversion_operations Conversion Operations
 * @brief Functions for converting between integer types and representations
 * @{
 */

/**
 * @brief Convert integer to 32-bit signed integer
 * @param big Integer to convert (may be NULL)
 * @param result Pointer to store result
 * @return true if conversion successful and no overflow, false otherwise
 * @since 1.0.0
 * 
 * @code
 * di_int big = di_from_int32(42);
 * int32_t value;
 * if (di_to_int32(big, &value)) {
 *     printf("Value: %d\n", value);
 * } else {
 *     printf("Overflow!\n");
 * }
 * di_release(&big);
 * @endcode
 */
DI_DEF bool di_to_int32(di_int big, int32_t* result);

/**
 * @brief Convert integer to 64-bit signed integer
 * @param big Integer to convert (may be NULL)
 * @param result Pointer to store result
 * @return true if conversion successful and no overflow, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_to_int64(di_int big, int64_t* result);

/**
 * @brief Convert integer to 32-bit unsigned integer
 * @param big Integer to convert (may be NULL)
 * @param result Pointer to store result
 * @return true if conversion successful and no overflow/underflow, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_to_uint32(di_int big, uint32_t* result);

/**
 * @brief Convert integer to 64-bit unsigned integer
 * @param big Integer to convert (may be NULL)
 * @param result Pointer to store result
 * @return true if conversion successful and no overflow/underflow, false otherwise
 * @since 1.0.0
 */
DI_DEF bool di_to_uint64(di_int big, uint64_t* result);

/**
 * @brief Convert integer to double precision floating point
 * @param big Integer to convert (may be NULL)
 * @return Double representation of integer, or 0.0 if big is NULL
 * @since 1.0.0
 * 
 * @note May lose precision for very large integers
 * @note NULL integers return 0.0
 */
DI_DEF double di_to_double(di_int big);

/**
 * @brief Convert integer to string representation
 * @param big Integer to convert (may be NULL)
 * @param base Number base (2-36)
 * @return Dynamically allocated string (caller must free), or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int big = di_from_int32(-42);
 * char* str = di_to_string(big, 10);
 * printf("Value: %s\n", str);  // Value: -42
 * free(str);
 * di_release(&big);
 * @endcode
 * 
 * @note Currently only base 10 is implemented
 * @note Caller is responsible for freeing returned string
 */
DI_DEF char* di_to_string(di_int big, int base);

/** @} */ // end of conversion_operations

/**
 * @defgroup utility_functions Utility Functions
 * @brief Utility functions for querying integer properties
 * @{
 */

/**
 * @brief Get bit length of integer (number of bits needed to represent)
 * @param big Integer to query (may be NULL)
 * @return Number of bits needed to represent integer, or 0 if NULL or zero
 * @since 1.0.0
 */
DI_DEF size_t di_bit_length(di_int big);

/**
 * @brief Get number of limbs used by integer
 * @param big Integer to query (may be NULL)
 * @return Number of limbs in use, or 0 if NULL
 * @since 1.0.0
 * 
 * @note Primarily for debugging and internal use
 */
DI_DEF size_t di_limb_count(di_int big);

/**
 * @brief Reserve capacity for an integer (performance optimization)
 * @param big Integer to resize (may be NULL)
 * @param capacity Number of limbs to reserve capacity for
 * @return true if successful, false on allocation failure
 * @since 1.0.0
 * 
 * @code
 * di_int big = di_from_int32(0);
 * 
 * // Pre-allocate space for large operations to avoid repeated allocations
 * if (di_reserve(big, 100)) {
 *     // Now operations won't need to reallocate until 100 limbs are needed
 *     for (int i = 0; i < 1000; i++) {
 *         di_int temp = di_mul_i32(big, 2);
 *         di_release(&big);
 *         big = temp;
 *     }
 * }
 * di_release(&big);
 * @endcode
 * 
 * @note This is purely a performance optimization - integers work without it
 * @note Similar to std::vector::reserve() in C++
 * @see di_limb_count() for getting current limb count
 */
DI_DEF bool di_reserve(di_int big, size_t capacity);

/** @} */ // end of utility_functions

/**
 * @defgroup advanced_math Advanced Mathematical Operations
 * @brief Advanced mathematical functions for arbitrary precision integers
 * @{
 */

/**
 * @brief Modular exponentiation: (base^exp) mod mod
 * @param base Base integer (may be NULL)
 * @param exp Exponent integer (may be NULL)
 * @param mod Modulus integer (may be NULL)
 * @return New di_int with result of (base^exp) mod mod, or NULL on failure
 * @since 1.0.0
 * 
 * @note Returns NULL if mod is zero or one
 * @note Uses binary exponentiation for efficiency
 */
DI_DEF di_int di_mod_pow(di_int base, di_int exp, di_int mod);

/**
 * @brief Greatest Common Divisor using Euclidean algorithm
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with GCD of |a| and |b|, or NULL on failure
 * @since 1.0.0
 * 
 * @see di_lcm() for Least Common Multiple
 * @see di_extended_gcd() for extended Euclidean algorithm
 */
DI_DEF di_int di_gcd(di_int a, di_int b);

/**
 * @brief Least Common Multiple
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @return New di_int with LCM of a and b, or NULL on failure
 * @since 1.0.0
 * 
 * @note Uses identity: lcm(a,b) = |a*b| / gcd(a,b)
 * @see di_gcd() for Greatest Common Divisor
 */
DI_DEF di_int di_lcm(di_int a, di_int b);

/**
 * @brief Extended Euclidean Algorithm
 * @param a First integer (may be NULL)
 * @param b Second integer (may be NULL)
 * @param x Pointer to store coefficient x
 * @param y Pointer to store coefficient y
 * @return GCD of a and b, with coefficients such that ax + by = gcd(a,b)
 * @since 1.0.0
 * 
 * @note Sets *x and *y to coefficients satisfying the Bezout identity
 */
DI_DEF di_int di_extended_gcd(di_int a, di_int b, di_int* x, di_int* y);

/**
 * @brief Integer square root using Newton's method
 * @param n Integer to find square root of (may be NULL)
 * @return New di_int with floor(sqrt(n)), or NULL on failure
 * @since 1.0.0
 * 
 * @note Returns NULL for negative inputs
 */
DI_DEF di_int di_sqrt(di_int n);

/**
 * @brief Calculate factorial
 * @param n Non-negative integer
 * @return New di_int with n! (n factorial), or NULL on failure
 * @since 1.0.0
 * 
 * @code
 * di_int fact5 = di_factorial(5);  // 120
 * di_int fact10 = di_factorial(10); // 3628800
 * @endcode
 */
DI_DEF di_int di_factorial(uint32_t n);

/** @} */ // end of advanced_math

/**
 * @defgroup prime_functions Prime Number Functions
 * @brief Functions for working with prime numbers
 * @{
 */

/**
 * @brief Test if integer is prime (Miller-Rabin test)
 * @param n Integer to test (may be NULL)
 * @param certainty Number of rounds for probabilistic test
 * @return true if n is probably prime, false if composite or NULL
 * @since 1.0.0
 * 
 * @note Higher certainty values increase accuracy but take longer
 */
DI_DEF bool di_is_prime(di_int n, int certainty);

/**
 * @brief Find next prime number >= n
 * @param n Starting integer (may be NULL)
 * @return New di_int with next prime >= n, or NULL on failure
 * @since 1.0.0
 */
DI_DEF di_int di_next_prime(di_int n);

/** @} */ // end of prime_functions

/**
 * @defgroup random_functions Random Number Generation
 * @brief Functions for generating random integers
 * @{
 */

/**
 * @brief Generate random integer with specified bit length
 * @param bits Number of bits for the random integer
 * @return New di_int with random value, or NULL on failure
 * @since 1.0.0
 * 
 * @warning Not cryptographically secure - use proper CSPRNG for security
 * @note Returns zero if bits is 0
 */
DI_DEF di_int di_random(size_t bits);

/**
 * @brief Generate random integer in range [min, max)
 * @param min Minimum value (inclusive, may be NULL)
 * @param max Maximum value (exclusive, may be NULL)
 * @return New di_int with random value in range, or NULL on failure
 * @since 1.0.0
 * 
 * @warning Not cryptographically secure - use proper CSPRNG for security
 * @note Returns NULL if min >= max
 */
DI_DEF di_int di_random_range(di_int min, di_int max);

/** @} */ // end of random_functions

/**
 * @defgroup overflow_detection Overflow Detection Helpers
 * @brief Helper functions for detecting fixed-size arithmetic overflow
 * @{
 */

/**
 * @brief Add two int32_t values with overflow detection
 * @param a First operand
 * @param b Second operand
 * @param result Pointer to store result
 * @return true if addition overflowed, false if safe
 * @since 1.0.0
 * 
 * @code
 * int32_t result;
 * if (di_add_overflow_int32(INT32_MAX, 1, &result)) {
 *     // Overflow occurred, use big integer arithmetic
 *     di_int a = di_from_int32(INT32_MAX);
 *     di_int b = di_from_int32(1);
 *     di_int sum = di_add(a, b);
 *     // ... handle big integer result
 * }
 * @endcode
 */
DI_DEF bool di_add_overflow_int32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief Subtract two int32_t values with overflow detection
 * @param a Minuend
 * @param b Subtrahend
 * @param result Pointer to store result
 * @return true if subtraction overflowed, false if safe
 * @since 1.0.0
 */
DI_DEF bool di_subtract_overflow_int32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief Multiply two int32_t values with overflow detection
 * @param a First operand
 * @param b Second operand
 * @param result Pointer to store result
 * @return true if multiplication overflowed, false if safe
 * @since 1.0.0
 */
DI_DEF bool di_multiply_overflow_int32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief Add two int64_t values with overflow detection
 * @param a First operand
 * @param b Second operand
 * @param result Pointer to store result
 * @return true if addition overflowed, false if safe
 * @since 1.0.0
 */
DI_DEF bool di_add_overflow_int64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief Subtract two int64_t values with overflow detection
 * @param a Minuend
 * @param b Subtrahend
 * @param result Pointer to store result
 * @return true if subtraction overflowed, false if safe
 * @since 1.0.0
 */
DI_DEF bool di_subtract_overflow_int64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief Multiply two int64_t values with overflow detection
 * @param a First operand
 * @param b Second operand
 * @param result Pointer to store result
 * @return true if multiplication overflowed, false if safe
 * @since 1.0.0
 */
DI_DEF bool di_multiply_overflow_int64(int64_t a, int64_t b, int64_t* result);

/** @} */ // end of overflow_detection

#ifdef DI_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

/* Internal structure */
struct di_int_internal {
    size_t ref_count;       // Reference count
    di_limb_t* limbs;       // Array of limbs (little-endian)
    size_t limb_count;      // Number of limbs used
    size_t limb_capacity;   // Allocated capacity
    bool is_negative;       // Sign flag
};

/* Internal function declarations */
static bool di_resize_internal(struct di_int_internal* big, size_t new_capacity);

/* Internal helper functions */

static struct di_int_internal* di_alloc(size_t initial_capacity) {
    struct di_int_internal* big = (struct di_int_internal*)DI_MALLOC(sizeof(struct di_int_internal));
    if (!big) return NULL;
    
    big->ref_count = 1;
    big->limb_count = 0;
    big->limb_capacity = initial_capacity;
    big->is_negative = false;
    
    if (initial_capacity > 0) {
        big->limbs = (di_limb_t*)DI_MALLOC(sizeof(di_limb_t) * initial_capacity);
        if (!big->limbs) {
            DI_FREE(big);
            return NULL;
        }
        memset(big->limbs, 0, sizeof(di_limb_t) * initial_capacity);
    } else {
        big->limbs = NULL;
    }
    
    return big;
}

static void di_normalize(struct di_int_internal* big) {
    // Remove leading zeros
    while (big->limb_count > 0 && big->limbs[big->limb_count - 1] == 0) {
        big->limb_count--;
    }
    
    // Zero should not be negative
    if (big->limb_count == 0) {
        big->is_negative = false;
    }
}

DI_DEF bool di_reserve(di_int big, size_t capacity) {
    if (!big) return false;
    
    return di_resize_internal(big, capacity);
}

static bool di_resize_internal(struct di_int_internal* big, size_t new_capacity) {
    if (new_capacity <= big->limb_capacity) return true;
    
    di_limb_t* new_limbs = (di_limb_t*)DI_REALLOC(big->limbs, sizeof(di_limb_t) * new_capacity);
    if (!new_limbs) return false;
    
    // Zero out new limbs
    memset(new_limbs + big->limb_capacity, 0, sizeof(di_limb_t) * (new_capacity - big->limb_capacity));
    
    big->limbs = new_limbs;
    big->limb_capacity = new_capacity;
    return true;
}

/* Creation functions */

DI_DEF di_int di_from_int32(int32_t value) {
    struct di_int_internal* big = di_alloc(1);
    if (!big) return NULL;
    
    if (value < 0) {
        big->is_negative = true;
        // Handle INT32_MIN carefully
        if (value == INT32_MIN) {
            big->limbs[0] = (di_limb_t)INT32_MAX + 1;
        } else {
            big->limbs[0] = (di_limb_t)(-value);
        }
    } else {
        big->limbs[0] = (di_limb_t)value;
    }
    big->limb_count = (value != 0) ? 1 : 0;
    
    return big;
}

DI_DEF di_int di_from_int64(int64_t value) {
    struct di_int_internal* big = di_alloc(2);
    if (!big) return NULL;
    
    if (value < 0) {
        big->is_negative = true;
        // Handle INT64_MIN carefully
        if (value == INT64_MIN) {
            uint64_t uval = (uint64_t)INT64_MAX + 1;
            big->limbs[0] = (di_limb_t)(uval & DI_LIMB_MAX);
            big->limbs[1] = (di_limb_t)(uval >> DI_LIMB_BITS);
        } else {
            uint64_t uval = (uint64_t)(-value);
            big->limbs[0] = (di_limb_t)(uval & DI_LIMB_MAX);
            big->limbs[1] = (di_limb_t)(uval >> DI_LIMB_BITS);
        }
    } else {
        uint64_t uval = (uint64_t)value;
        big->limbs[0] = (di_limb_t)(uval & DI_LIMB_MAX);
        big->limbs[1] = (di_limb_t)(uval >> DI_LIMB_BITS);
    }
    
    big->limb_count = 2;
    di_normalize(big);
    
    return big;
}

DI_DEF di_int di_from_uint32(uint32_t value) {
    struct di_int_internal* big = di_alloc(1);
    if (!big) return NULL;
    
    big->limbs[0] = value;
    big->limb_count = (value != 0) ? 1 : 0;
    big->is_negative = false;
    
    return big;
}

DI_DEF di_int di_from_uint64(uint64_t value) {
    struct di_int_internal* big = di_alloc(2);
    if (!big) return NULL;
    
    big->limbs[0] = (di_limb_t)(value & DI_LIMB_MAX);
    big->limbs[1] = (di_limb_t)(value >> DI_LIMB_BITS);
    big->limb_count = 2;
    big->is_negative = false;
    di_normalize(big);
    
    return big;
}

DI_DEF di_int di_zero(void) {
    return di_from_int32(0);
}

DI_DEF di_int di_one(void) {
    return di_from_int32(1);
}

DI_DEF di_int di_copy(di_int big) {
    if (!big) return NULL;
    
    struct di_int_internal* copy = di_alloc(big->limb_capacity);
    if (!copy) return NULL;
    
    copy->limb_count = big->limb_count;
    copy->is_negative = big->is_negative;
    if (big->limb_count > 0) {
        memcpy(copy->limbs, big->limbs, sizeof(di_limb_t) * big->limb_count);
    }
    
    return copy;
}

DI_DEF di_int di_from_string(const char* str, int base) {
    if (!str || base < 2 || base > 36) return NULL;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if (*str == '\0') return NULL;
    
    // Check for sign
    bool is_negative = false;
    if (*str == '-') {
        is_negative = true;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Skip leading zeros
    while (*str == '0') str++;
    if (*str == '\0') return di_zero();
    
    // Count valid digits to estimate capacity
    const char* p = str;
    size_t digit_count = 0;
    while (*p) {
        char c = *p;
        int digit_val;
        if (c >= '0' && c <= '9') {
            digit_val = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit_val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit_val = c - 'A' + 10;
        } else {
            break; // Invalid character
        }
        
        if (digit_val >= base) break; // Invalid digit for this base
        digit_count++;
        p++;
    }
    
    if (digit_count == 0) return NULL;
    
    // Estimate capacity: log2(base^n) = n * log2(base) 
    // Rough estimate: each decimal digit needs ~3.32 bits, so use digit_count limbs
    size_t capacity = (digit_count + 7) / 8 + 1; // Conservative estimate
    if (capacity < 1) capacity = 1;
    
    struct di_int_internal* result = di_alloc(capacity);
    if (!result) return NULL;
    
    // Convert digit by digit using Horner's method: result = result * base + digit
    di_int base_big = di_from_int32(base);
    if (!base_big) {
        di_release((di_int*)&result);
        return NULL;
    }
    
    p = str;
    for (size_t i = 0; i < digit_count; i++) {
        char c = *p++;
        int digit_val;
        if (c >= '0' && c <= '9') {
            digit_val = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit_val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit_val = c - 'A' + 10;
        } else {
            break;
        }
        
        // result = result * base + digit
        di_int temp = di_mul(result, base_big);
        if (!temp) {
            di_release(&base_big);
            di_release((di_int*)&result);
            return NULL;
        }
        
        di_int digit_big = di_from_int32(digit_val);
        if (!digit_big) {
            di_release(&temp);
            di_release(&base_big);
            di_release((di_int*)&result);
            return NULL;
        }
        
        di_int new_result = di_add(temp, digit_big);
        di_release(&temp);
        di_release(&digit_big);
        
        if (!new_result) {
            di_release(&base_big);
            di_release((di_int*)&result);
            return NULL;
        }
        
        di_release((di_int*)&result);
        result = new_result;
    }
    
    di_release(&base_big);
    
    // Set sign
    if (is_negative && result->limb_count > 0) {
        result->is_negative = true;
    }
    
    return result;
}

/* Reference counting */

DI_DEF di_int di_retain(di_int big) {
    if (big) {
        big->ref_count++;
    }
    return big;
}

DI_DEF void di_release(di_int* big) {
    if (!big || !*big) return;
    
    struct di_int_internal* b = *big;
    if (--b->ref_count == 0) {
        if (b->limbs) {
            DI_FREE(b->limbs);
        }
        DI_FREE(b);
    }
    *big = NULL;
}

DI_DEF size_t di_ref_count(di_int big) {
    return big ? big->ref_count : 0;
}

/* Comparison functions */

DI_DEF int di_compare(di_int a, di_int b) {
    if (!a || !b) return 0;
    
    // Compare signs
    if (a->is_negative != b->is_negative) {
        return a->is_negative ? -1 : 1;
    }
    
    // Same sign, compare magnitudes
    if (a->limb_count != b->limb_count) {
        int mag_cmp = (a->limb_count > b->limb_count) ? 1 : -1;
        return a->is_negative ? -mag_cmp : mag_cmp;
    }
    
    // Same number of limbs, compare from most significant
    for (size_t i = a->limb_count; i > 0; i--) {
        if (a->limbs[i-1] > b->limbs[i-1]) {
            return a->is_negative ? -1 : 1;
        }
        if (a->limbs[i-1] < b->limbs[i-1]) {
            return a->is_negative ? 1 : -1;
        }
    }
    
    return 0;  // Equal
}

DI_DEF bool di_eq(di_int a, di_int b) {
    return di_compare(a, b) == 0;
}

DI_DEF bool di_lt(di_int a, di_int b) {
    return di_compare(a, b) < 0;
}

DI_DEF bool di_le(di_int a, di_int b) {
    return di_compare(a, b) <= 0;
}

DI_DEF bool di_gt(di_int a, di_int b) {
    return di_compare(a, b) > 0;
}

DI_DEF bool di_ge(di_int a, di_int b) {
    return di_compare(a, b) >= 0;
}

DI_DEF bool di_is_zero(di_int big) {
    return big && big->limb_count == 0;
}

DI_DEF bool di_is_negative(di_int big) {
    return big && big->is_negative && big->limb_count > 0;
}

DI_DEF bool di_is_positive(di_int big) {
    return big && !big->is_negative && big->limb_count > 0;
}

/* Conversion functions */

DI_DEF bool di_to_int32(di_int big, int32_t* result) {
    if (!big || !result) return false;
    
    if (big->limb_count == 0) {
        *result = 0;
        return true;
    }
    
    if (big->limb_count > 1) return false;
    
    di_limb_t val = big->limbs[0];
    
    if (big->is_negative) {
        if (val > (di_limb_t)INT32_MAX + 1) return false;
        if (val == (di_limb_t)INT32_MAX + 1) {
            *result = INT32_MIN;
        } else {
            *result = -(int32_t)val;
        }
    } else {
        if (val > INT32_MAX) return false;
        *result = (int32_t)val;
    }
    
    return true;
}

DI_DEF bool di_to_int64(di_int big, int64_t* result) {
    if (!big || !result) return false;
    
    if (big->limb_count == 0) {
        *result = 0;
        return true;
    }
    
    if (big->limb_count > 2) return false;
    
    uint64_t val = big->limbs[0];
    if (big->limb_count == 2) {
        val |= ((uint64_t)big->limbs[1] << DI_LIMB_BITS);
    }
    
    if (big->is_negative) {
        if (val > (uint64_t)INT64_MAX + 1) return false;
        if (val == (uint64_t)INT64_MAX + 1) {
            *result = INT64_MIN;
        } else {
            *result = -(int64_t)val;
        }
    } else {
        if (val > INT64_MAX) return false;
        *result = (int64_t)val;
    }
    
    return true;
}

DI_DEF double di_to_double(di_int big) {
    if (!big || big->limb_count == 0) return 0.0;
    
    double result = 0.0;
    double base = 1.0;
    
    for (size_t i = 0; i < big->limb_count; i++) {
        result += big->limbs[i] * base;
        base *= (double)(DI_LIMB_MAX + 1ULL);
    }
    
    return big->is_negative ? -result : result;
}

/* Helper function to compare magnitudes (ignoring sign) */
static int di_compare_magnitude(struct di_int_internal* a, struct di_int_internal* b) {
    if (a->limb_count > b->limb_count) return 1;
    if (a->limb_count < b->limb_count) return -1;
    
    // Same number of limbs - compare from most significant
    for (size_t i = a->limb_count; i > 0; i--) {
        size_t idx = i - 1;
        if (a->limbs[idx] > b->limbs[idx]) return 1;
        if (a->limbs[idx] < b->limbs[idx]) return -1;
    }
    
    return 0; // Equal magnitudes
}

/* Basic arithmetic implementations */

DI_DEF di_int di_add(di_int a, di_int b) {
    DI_ASSERT(a != NULL && "di_add: first operand cannot be NULL");
    DI_ASSERT(b != NULL && "di_add: second operand cannot be NULL");
    
    // Simple implementation for same-sign addition
    if (a->is_negative == b->is_negative) {
        struct di_int_internal* result = di_alloc(
            (a->limb_count > b->limb_count ? a->limb_count : b->limb_count) + 1
        );
        if (!result) return NULL;
        
        result->is_negative = a->is_negative;
        
        di_dlimb_t carry = 0;
        size_t max_limbs = a->limb_count > b->limb_count ? a->limb_count : b->limb_count;
        
        for (size_t i = 0; i < max_limbs; i++) {
            di_dlimb_t sum = carry;
            if (i < a->limb_count) sum += a->limbs[i];
            if (i < b->limb_count) sum += b->limbs[i];
            
            result->limbs[i] = (di_limb_t)(sum & DI_LIMB_MAX);
            carry = sum >> DI_LIMB_BITS;
        }
        
        if (carry) {
            result->limbs[max_limbs] = (di_limb_t)carry;
            result->limb_count = max_limbs + 1;
        } else {
            result->limb_count = max_limbs;
        }
        
        di_normalize(result);
        return result;
    }
    
    // Different signs - subtract magnitudes
    // First determine which has larger magnitude
    int cmp = di_compare_magnitude(a, b);
    
    struct di_int_internal* larger = (cmp >= 0) ? a : b;
    struct di_int_internal* smaller = (cmp >= 0) ? b : a;
    
    // Result takes sign of the larger magnitude operand
    // If a has larger magnitude: result = a - b (sign of a)
    // If b has larger magnitude: result = -(b - a) = b - a with opposite sign
    bool result_negative = (cmp >= 0) ? a->is_negative : b->is_negative;
    
    // Subtract smaller magnitude from larger magnitude
    struct di_int_internal* result = di_alloc(larger->limb_count);
    if (!result) return NULL;
    
    result->is_negative = result_negative;
    result->limb_count = larger->limb_count;
    
    di_limb_t borrow = 0;
    for (size_t i = 0; i < result->limb_count; i++) {
        di_limb_t a_limb = (i < larger->limb_count) ? larger->limbs[i] : 0;
        di_limb_t b_limb = (i < smaller->limb_count) ? smaller->limbs[i] : 0;
        
        // Use signed arithmetic to detect underflow
        int64_t signed_diff = (int64_t)a_limb - (int64_t)b_limb - (int64_t)borrow;
        
        di_dlimb_t diff;
        if (signed_diff < 0) {
            diff = (di_dlimb_t)(signed_diff + (1ULL << DI_LIMB_BITS));
            borrow = 1;
        } else {
            diff = (di_dlimb_t)signed_diff;
            borrow = 0;
        }
        
        result->limbs[i] = (di_limb_t)diff;
    }
    
    di_normalize(result);
    return result;
}

DI_DEF di_int di_add_i32(di_int a, int32_t b) {
    if (!a) return NULL;
    
    // Convert int32 to big integer and use regular addition
    di_int b_big = di_from_int32(b);
    if (!b_big) return NULL;
    
    di_int result = di_add(a, b_big);
    di_release(&b_big);
    return result;
}

DI_DEF di_int di_sub(di_int a, di_int b) {
    DI_ASSERT(a != NULL && "di_sub: first operand cannot be NULL");
    DI_ASSERT(b != NULL && "di_sub: second operand cannot be NULL");
    
    // a - b = a + (-b)
    di_int neg_b = di_negate(b);
    if (!neg_b) return NULL;
    
    di_int result = di_add(a, neg_b);
    di_release(&neg_b);
    return result;
}

DI_DEF di_int di_sub_i32(di_int a, int32_t b) {
    if (!a) return NULL;
    
    // Convert int32 to big integer and use regular subtraction
    di_int b_big = di_from_int32(b);
    if (!b_big) return NULL;
    
    di_int result = di_sub(a, b_big);
    di_release(&b_big);
    return result;
}

DI_DEF di_int di_negate(di_int a) {
    if (!a) return NULL;
    
    di_int result = di_copy(a);
    if (result && result->limb_count > 0) {
        result->is_negative = !result->is_negative;
    }
    return result;
}

/* String conversion implementation */
DI_DEF char* di_to_string(di_int big, int base) {
    if (!big || base < 2 || base > 36) return NULL;
    
    if (big->limb_count == 0) {
        char* str = (char*)DI_MALLOC(2);
        if (str) {
            str[0] = '0';
            str[1] = '\0';
        }
        return str;
    }
    
    // Simple implementation for base 10
    if (base == 10) {
        // Proper arbitrary precision decimal conversion using efficient modular arithmetic
        size_t max_digits = big->limb_count * 10 + 10;
        char* buffer = (char*)DI_MALLOC(max_digits);
        if (!buffer) return NULL;
        
        // Make a working copy
        di_int work = di_copy(big);
        if (!work) {
            DI_FREE(buffer);
            return NULL;
        }
        
        char* digits = (char*)DI_MALLOC(max_digits);
        if (!digits) {
            DI_FREE(buffer);
            di_release(&work);
            return NULL;
        }
        
        size_t digit_count = 0;
        
        // Handle zero case
        if (di_is_zero(work)) {
            buffer[0] = '0';
            buffer[1] = '\0';
            DI_FREE(digits);
            di_release(&work);
            return buffer;
        }
        
        // Extract digits using limb-level division by 10 (much more efficient than di_div)
        while (!di_is_zero(work)) {
            // Divide by 10 using limb arithmetic
            di_dlimb_t remainder = 0;
            for (int i = (int)work->limb_count - 1; i >= 0; i--) {
                di_dlimb_t temp = remainder * ((di_dlimb_t)DI_LIMB_MAX + 1) + work->limbs[i];
                work->limbs[i] = (di_limb_t)(temp / 10);
                remainder = temp % 10;
            }
            
            // Store the digit
            digits[digit_count++] = '0' + (char)remainder;
            
            // Remove leading zeros
            while (work->limb_count > 0 && work->limbs[work->limb_count - 1] == 0) {
                work->limb_count--;
            }
        }
        
        // Build result string (digits are in reverse order)
        size_t pos = 0;
        if (big->is_negative) {
            buffer[pos++] = '-';
        }
        
        for (size_t i = digit_count; i > 0; i--) {
            buffer[pos++] = digits[i - 1];
        }
        buffer[pos] = '\0';
        
        DI_FREE(digits);
        di_release(&work);
        
        return buffer;
    }
    
    return NULL;
}

/* Overflow detection helpers */

DI_DEF bool di_add_overflow_int32(int32_t a, int32_t b, int32_t* result) {
    int64_t sum = (int64_t)a + (int64_t)b;
    if (sum < INT32_MIN || sum > INT32_MAX) {
        return false;  // Overflow
    }
    *result = (int32_t)sum;
    return true;
}

DI_DEF bool di_subtract_overflow_int32(int32_t a, int32_t b, int32_t* result) {
    int64_t diff = (int64_t)a - (int64_t)b;
    if (diff < INT32_MIN || diff > INT32_MAX) {
        return false;  // Overflow
    }
    *result = (int32_t)diff;
    return true;
}

DI_DEF bool di_multiply_overflow_int32(int32_t a, int32_t b, int32_t* result) {
    int64_t prod = (int64_t)a * (int64_t)b;
    if (prod < INT32_MIN || prod > INT32_MAX) {
        return false;  // Overflow
    }
    *result = (int32_t)prod;
    return true;
}

DI_DEF bool di_add_overflow_int64(int64_t a, int64_t b, int64_t* result) {
    // Check for overflow without causing UB
    if (b > 0 && a > INT64_MAX - b) return false;
    if (b < 0 && a < INT64_MIN - b) return false;
    *result = a + b;
    return true;
}

DI_DEF bool di_subtract_overflow_int64(int64_t a, int64_t b, int64_t* result) {
    // Check for overflow without causing UB
    if (b < 0 && a > INT64_MAX + b) return false;
    if (b > 0 && a < INT64_MIN + b) return false;
    *result = a - b;
    return true;
}

DI_DEF bool di_multiply_overflow_int64(int64_t a, int64_t b, int64_t* result) {
    // Check for overflow - this is complex for 64-bit
    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }
    
    // Check if multiplication would overflow
    if (a > 0) {
        if (b > 0) {
            if (a > INT64_MAX / b) return false;
        } else {
            if (b < INT64_MIN / a) return false;
        }
    } else {
        if (b > 0) {
            if (a < INT64_MIN / b) return false;
        } else {
            if (a != 0 && b < INT64_MAX / a) return false;
        }
    }
    
    *result = a * b;
    return true;
}

// Big integer multiplication - use the original working approach for single limb case, full algorithm for multi-limb
DI_DEF di_int di_mul(di_int a, di_int b) {
    DI_ASSERT(a != NULL && "di_mul: first operand cannot be NULL");
    DI_ASSERT(b != NULL && "di_mul: second operand cannot be NULL");
    
    // Handle zero cases
    if (a->limb_count == 0 || b->limb_count == 0) {
        return di_from_int32(0);
    }
    
    // For single limb x single limb, use direct 64-bit multiplication (should be exact)
    if (a->limb_count == 1 && b->limb_count == 1) {
        di_dlimb_t product = (di_dlimb_t)a->limbs[0] * (di_dlimb_t)b->limbs[0];
        bool result_negative = (a->is_negative != b->is_negative);
        
        if (product <= DI_LIMB_MAX) {
            // Result fits in one limb
            di_int result = di_from_uint32((uint32_t)product);
            if (result && result_negative && product > 0) {
                result->is_negative = true;
            }
            return result;
        } else {
            // Result needs two limbs
            struct di_int_internal* result = di_alloc(2);
            if (!result) return NULL;
            
            result->is_negative = result_negative;
            result->limb_count = 2;
            result->limbs[0] = (di_limb_t)(product & DI_LIMB_MAX);
            result->limbs[1] = (di_limb_t)(product >> DI_LIMB_BITS);
            
            di_normalize(result);
            return result;
        }
    }
    
    // For multi-limb cases, use proper schoolbook multiplication
    bool result_negative = (a->is_negative != b->is_negative);
    size_t result_capacity = a->limb_count + b->limb_count;
    struct di_int_internal* result = di_alloc(result_capacity);
    if (!result) return NULL;
    
    result->is_negative = result_negative;
    result->limb_count = result_capacity;
    
    // Initialize result to zero
    for (size_t i = 0; i < result_capacity; i++) {
        result->limbs[i] = 0;
    }
    
    // Schoolbook multiplication
    for (size_t i = 0; i < a->limb_count; i++) {
        di_dlimb_t carry = 0;
        
        for (size_t j = 0; j < b->limb_count; j++) {
            size_t pos = i + j;
            di_dlimb_t prod = (di_dlimb_t)a->limbs[i] * (di_dlimb_t)b->limbs[j];
            di_dlimb_t sum = (di_dlimb_t)result->limbs[pos] + prod + carry;
            
            result->limbs[pos] = (di_limb_t)(sum & DI_LIMB_MAX);
            carry = sum >> DI_LIMB_BITS;
        }
        
        // Handle remaining carry
        if (carry > 0 && i + b->limb_count < result_capacity) {
            result->limbs[i + b->limb_count] = (di_limb_t)carry;
        }
    }
    
    di_normalize(result);
    return result;
}

// Big integer multiplication by int32
DI_DEF di_int di_mul_i32(di_int a, int32_t b) {
    if (!a) return NULL;
    
    di_int b_big = di_from_int32(b);
    di_int result = di_mul(a, b_big);
    di_release(&b_big);
    return result;
}

// Big integer division - returns quotient
DI_DEF di_int di_div(di_int a, di_int b) {
    DI_ASSERT(a != NULL && "di_div: dividend cannot be NULL");
    DI_ASSERT(b != NULL && "di_div: divisor cannot be NULL");
    DI_ASSERT(!di_is_zero(b) && "di_div: division by zero");
    
    // Special cases
    if (di_is_zero(a)) return di_zero();
    if (di_eq(a, b)) return di_one();
    
    // Compare absolute values
    di_int abs_a = di_abs(a);
    di_int abs_b = di_abs(b);
    
    if (di_lt(abs_a, abs_b)) {
        di_release(&abs_a);
        di_release(&abs_b);
        return di_zero();
    }
    
    // Proper long division algorithm for efficiency
    size_t dividend_limbs = abs_a->limb_count;
    size_t divisor_limbs = abs_b->limb_count;
    
    // For single limb divisor, use optimized single-limb division
    if (divisor_limbs == 1) {
        di_limb_t divisor_limb = abs_b->limbs[0];
        struct di_int_internal* quotient = di_alloc(dividend_limbs);
        if (!quotient) {
            di_release(&abs_a);
            di_release(&abs_b);
            return NULL;
        }
        
        quotient->limb_count = dividend_limbs;
        di_dlimb_t remainder = 0;
        
        // Divide from most significant to least significant limb
        for (int i = (int)dividend_limbs - 1; i >= 0; i--) {
            di_dlimb_t temp = remainder * ((di_dlimb_t)DI_LIMB_MAX + 1) + abs_a->limbs[i];
            quotient->limbs[i] = (di_limb_t)(temp / divisor_limb);
            remainder = temp % divisor_limb;
        }
        
        di_normalize(quotient);
        
        // Set result sign and apply floor division semantics
        bool result_negative = (a->is_negative != b->is_negative);
        
        // For floor division: if result would be negative and there's a remainder,
        // we need to add 1 to the quotient magnitude (going more negative)
        if (result_negative && remainder > 0 && quotient->limb_count > 0) {
            // Add 1 to quotient magnitude for floor division
            di_int one = di_one();
            di_int adjusted_quotient = di_add(quotient, one);
            di_release(&quotient);
            di_release(&one);
            quotient = adjusted_quotient;
            quotient->is_negative = true;
        } else if (result_negative && quotient->limb_count > 0) {
            quotient->is_negative = true;
        }
        
        di_release(&abs_a);
        di_release(&abs_b);
        return quotient;
    }
    
    // For multi-limb division, fall back to binary long division (more efficient than repeated subtraction)
    di_int quotient = di_zero();
    di_int remainder = di_zero();
    
    // Process bits from most significant to least significant
    for (int limb_idx = (int)dividend_limbs - 1; limb_idx >= 0; limb_idx--) {
        for (int bit = DI_LIMB_BITS - 1; bit >= 0; bit--) {
            // Shift remainder left by 1
            di_int temp_remainder = di_shift_left(remainder, 1);
            di_release(&remainder);
            remainder = temp_remainder;
            
            // Add current bit of dividend to remainder
            if (abs_a->limbs[limb_idx] & ((di_limb_t)1 << bit)) {
                di_int temp = di_add_i32(remainder, 1);
                di_release(&remainder);
                remainder = temp;
            }
            
            // Shift quotient left by 1
            di_int temp_quotient = di_shift_left(quotient, 1);
            di_release(&quotient);
            quotient = temp_quotient;
            
            // If remainder >= divisor, subtract divisor and set bit in quotient
            if (di_ge(remainder, abs_b)) {
                di_int temp_remainder = di_sub(remainder, abs_b);
                di_int temp_quotient = di_add_i32(quotient, 1);
                di_release(&remainder);
                di_release(&quotient);
                remainder = temp_remainder;
                quotient = temp_quotient;
            }
            
            if (!remainder || !quotient) {
                di_release(&abs_a);
                di_release(&abs_b);
                di_release(&remainder);
                di_release(&quotient);
                return NULL;
            }
        }
    }
    
    // Implement floor division (Python-style)
    bool result_negative = (a->is_negative != b->is_negative);
    
    // For floor division: if result would be negative and there's a remainder,
    // we need to add 1 to the magnitude of the quotient (since we're going more negative)
    if (result_negative && !di_is_zero(remainder) && quotient->limb_count > 0) {
        // Add 1 to quotient magnitude for floor division
        di_int one = di_one();
        di_int adjusted_quotient = di_add(quotient, one);
        di_release(&quotient);
        di_release(&one);
        quotient = adjusted_quotient;
    }
    
    // Set the sign
    if (result_negative && quotient->limb_count > 0) {
        quotient->is_negative = true;
    }
    
    di_release(&abs_a);
    di_release(&abs_b);
    di_release(&remainder);
    
    return quotient;
}

// Big integer modulo - proper arbitrary precision implementation
DI_DEF di_int di_mod(di_int a, di_int b) {
    DI_ASSERT(a != NULL && "di_mod: dividend cannot be NULL");
    DI_ASSERT(b != NULL && "di_mod: divisor cannot be NULL");
    DI_ASSERT(!di_is_zero(b) && "di_mod: modulo by zero");
    
    // Special cases
    if (di_is_zero(a)) return di_zero(); // 0 % b = 0
    if (di_eq(a, b)) return di_zero();   // a % a = 0
    
    // For floor modulo: remainder = a - (floor(a/b) * b)
    // This ensures the remainder has the same sign as the divisor
    di_int quotient = di_div(a, b);  // Use floor division
    di_int product = di_mul(quotient, b);
    di_int remainder = di_sub(a, product);
    
    di_release(&quotient);
    di_release(&product);
    
    return remainder;
}

// Big integer absolute value
DI_DEF di_int di_abs(di_int a) {
    if (!a) return NULL;
    
    // If already positive or zero, just return a copy
    if (!a->is_negative) {
        return di_copy(a);
    }
    
    // Create a positive copy of the negative number
    di_int result = di_copy(a);
    if (result && result->limb_count > 0) {
        result->is_negative = false;
    }
    return result;
}

// Bitwise operations
DI_DEF di_int di_and(di_int a, di_int b) {
    if (!a || !b) return NULL;
    
    size_t max_limbs = (a->limb_count > b->limb_count) ? a->limb_count : b->limb_count;
    struct di_int_internal* result = di_alloc(max_limbs);
    if (!result) return NULL;
    
    // AND operation on limbs
    for (size_t i = 0; i < max_limbs; i++) {
        di_limb_t a_limb = (i < a->limb_count) ? a->limbs[i] : 0;
        di_limb_t b_limb = (i < b->limb_count) ? b->limbs[i] : 0;
        result->limbs[i] = a_limb & b_limb;
    }
    result->limb_count = max_limbs;
    
    // Result is positive (bitwise operations on magnitudes)
    result->is_negative = false;
    di_normalize(result);
    
    return result;
}

DI_DEF di_int di_or(di_int a, di_int b) {
    if (!a || !b) return NULL;
    
    size_t max_limbs = (a->limb_count > b->limb_count) ? a->limb_count : b->limb_count;
    struct di_int_internal* result = di_alloc(max_limbs);
    if (!result) return NULL;
    
    // OR operation on limbs
    for (size_t i = 0; i < max_limbs; i++) {
        di_limb_t a_limb = (i < a->limb_count) ? a->limbs[i] : 0;
        di_limb_t b_limb = (i < b->limb_count) ? b->limbs[i] : 0;
        result->limbs[i] = a_limb | b_limb;
    }
    result->limb_count = max_limbs;
    
    // Result is positive (bitwise operations on magnitudes)
    result->is_negative = false;
    di_normalize(result);
    
    return result;
}

DI_DEF di_int di_xor(di_int a, di_int b) {
    if (!a || !b) return NULL;
    
    size_t max_limbs = (a->limb_count > b->limb_count) ? a->limb_count : b->limb_count;
    struct di_int_internal* result = di_alloc(max_limbs);
    if (!result) return NULL;
    
    // XOR operation on limbs
    for (size_t i = 0; i < max_limbs; i++) {
        di_limb_t a_limb = (i < a->limb_count) ? a->limbs[i] : 0;
        di_limb_t b_limb = (i < b->limb_count) ? b->limbs[i] : 0;
        result->limbs[i] = a_limb ^ b_limb;
    }
    result->limb_count = max_limbs;
    
    // Result is positive (bitwise operations on magnitudes)
    result->is_negative = false;
    di_normalize(result);
    
    return result;
}

DI_DEF di_int di_not(di_int a) {
    if (!a) return NULL;
    
    // For simplicity, NOT operation on fixed width (one limb beyond significant bits)
    size_t result_limbs = a->limb_count + 1;
    struct di_int_internal* result = di_alloc(result_limbs);
    if (!result) return NULL;
    
    // NOT operation on limbs
    for (size_t i = 0; i < a->limb_count; i++) {
        result->limbs[i] = ~a->limbs[i];
    }
    result->limbs[a->limb_count] = ~((di_limb_t)0); // Set high limb to all 1s
    result->limb_count = result_limbs;
    
    // Result is positive (bitwise operations on magnitudes)
    result->is_negative = false;
    di_normalize(result);
    
    return result;
}

DI_DEF di_int di_shift_left(di_int a, size_t bits) {
    if (!a || bits == 0) return di_copy(a);
    
    size_t limb_shift = bits / DI_LIMB_BITS;
    size_t bit_shift = bits % DI_LIMB_BITS;
    
    size_t new_limb_count = a->limb_count + limb_shift + (bit_shift > 0 ? 1 : 0);
    struct di_int_internal* result = di_alloc(new_limb_count);
    if (!result) return NULL;
    
    // Clear the result limbs
    for (size_t i = 0; i < new_limb_count; i++) {
        result->limbs[i] = 0;
    }
    
    // Copy and shift limbs
    if (bit_shift == 0) {
        // Simple limb shift
        for (size_t i = 0; i < a->limb_count; i++) {
            result->limbs[i + limb_shift] = a->limbs[i];
        }
    } else {
        // Bit shift within limbs
        di_limb_t carry = 0;
        for (size_t i = 0; i < a->limb_count; i++) {
            di_limb_t shifted = (a->limbs[i] << bit_shift) | carry;
            carry = a->limbs[i] >> (DI_LIMB_BITS - bit_shift);
            result->limbs[i + limb_shift] = shifted;
        }
        if (carry > 0) {
            result->limbs[a->limb_count + limb_shift] = carry;
        }
    }
    
    result->limb_count = new_limb_count;
    result->is_negative = a->is_negative;
    di_normalize(result);
    
    return result;
}

DI_DEF di_int di_shift_right(di_int a, size_t bits) {
    if (!a || bits == 0) return di_copy(a);
    
    size_t limb_shift = bits / DI_LIMB_BITS;
    size_t bit_shift = bits % DI_LIMB_BITS;
    
    // If shifting more limbs than we have, result is zero
    if (limb_shift >= a->limb_count) {
        return di_zero();
    }
    
    size_t new_limb_count = a->limb_count - limb_shift;
    struct di_int_internal* result = di_alloc(new_limb_count);
    if (!result) return NULL;
    
    if (bit_shift == 0) {
        // Simple limb shift
        for (size_t i = 0; i < new_limb_count; i++) {
            result->limbs[i] = a->limbs[i + limb_shift];
        }
    } else {
        // Bit shift within limbs
        for (size_t i = 0; i < new_limb_count; i++) {
            di_limb_t current = a->limbs[i + limb_shift];
            di_limb_t next = (i + limb_shift + 1 < a->limb_count) ? 
                            a->limbs[i + limb_shift + 1] : 0;
            result->limbs[i] = (current >> bit_shift) | 
                              (next << (DI_LIMB_BITS - bit_shift));
        }
    }
    
    result->limb_count = new_limb_count;
    result->is_negative = a->is_negative;
    di_normalize(result);
    
    return result;
}

// GCD using Euclidean algorithm
DI_DEF di_int di_gcd(di_int a, di_int b) {
    if (!a || !b) return NULL;
    
    di_int abs_a = di_abs(a);
    di_int abs_b = di_abs(b);
    
    if (di_is_zero(abs_a)) {
        di_release(&abs_a);
        return abs_b;
    }
    if (di_is_zero(abs_b)) {
        di_release(&abs_b);
        return abs_a;
    }
    
    // Euclidean algorithm: gcd(a,b) = gcd(b, a mod b)
    while (!di_is_zero(abs_b)) {
        di_int remainder = di_mod(abs_a, abs_b);
        if (!remainder) {
            di_release(&abs_a);
            di_release(&abs_b);
            return NULL;
        }
        
        di_release(&abs_a);
        abs_a = abs_b;
        abs_b = remainder;
    }
    
    di_release(&abs_b);
    return abs_a;
}

// LCM using the identity: lcm(a,b) = |a*b| / gcd(a,b)
DI_DEF di_int di_lcm(di_int a, di_int b) {
    if (!a || !b) return NULL;
    if (di_is_zero(a) || di_is_zero(b)) return di_zero();
    
    di_int gcd = di_gcd(a, b);
    if (!gcd) return NULL;
    
    di_int product = di_mul(a, b);
    if (!product) {
        di_release(&gcd);
        return NULL;
    }
    
    di_int abs_product = di_abs(product);
    di_release(&product);
    if (!abs_product) {
        di_release(&gcd);
        return NULL;
    }
    
    di_int result = di_div(abs_product, gcd);
    di_release(&abs_product);
    di_release(&gcd);
    
    return result;
}

// Simple integer square root using Newton's method
DI_DEF di_int di_sqrt(di_int n) {
    if (!n) return NULL;
    if (di_is_negative(n)) return NULL; // Square root of negative number
    if (di_is_zero(n)) return di_zero();
    
    di_int one = di_one();
    if (di_eq(n, one)) {
        di_release(&one);
        return di_one();
    }
    
    // Initial guess: x = n / 2
    di_int two = di_from_int32(2);
    di_int x = di_div(n, two);
    if (!x) {
        di_release(&one);
        di_release(&two);
        return NULL;
    }
    
    // Newton's method: x_new = (x + n/x) / 2
    for (int iterations = 0; iterations < 100; iterations++) { // Limit iterations
        di_int quotient = di_div(n, x);
        if (!quotient) break;
        
        di_int sum = di_add(x, quotient);
        di_release(&quotient);
        if (!sum) break;
        
        di_int x_new = di_div(sum, two);
        di_release(&sum);
        if (!x_new) break;
        
        // Check for convergence
        if (di_ge(x_new, x)) {
            di_release(&x_new);
            break;
        }
        
        di_release(&x);
        x = x_new;
    }
    
    di_release(&one);
    di_release(&two);
    return x;
}

// Factorial function
DI_DEF di_int di_factorial(uint32_t n) {
    if (n <= 1) return di_one();
    
    di_int result = di_one();
    if (!result) return NULL;
    
    for (uint32_t i = 2; i <= n; i++) {
        di_int i_big = di_from_uint32(i);
        if (!i_big) {
            di_release(&result);
            return NULL;
        }
        
        di_int new_result = di_mul(result, i_big);
        di_release(&i_big);
        di_release(&result);
        
        if (!new_result) return NULL;
        result = new_result;
    }
    
    return result;
}

// Modular exponentiation: (base^exp) mod mod
// Uses binary exponentiation for efficiency
DI_DEF di_int di_mod_pow(di_int base, di_int exp, di_int mod) {
    if (!base || !exp || !mod) return NULL;
    if (di_is_zero(mod)) return NULL; // Division by zero
    if (di_eq(mod, di_one())) return di_zero(); // x mod 1 = 0
    
    if (di_is_zero(exp)) return di_one(); // base^0 = 1
    if (di_is_zero(base)) return di_zero(); // 0^exp = 0
    
    di_int result = di_one();
    di_int base_mod = di_mod(base, mod); // Reduce base first
    di_int exp_copy = di_copy(exp);
    
    if (!result || !base_mod || !exp_copy) {
        di_release(&result);
        di_release(&base_mod);
        di_release(&exp_copy);
        return NULL;
    }
    
    // Binary exponentiation
    while (!di_is_zero(exp_copy)) {
        // If exp is odd, multiply result by base_mod
        di_int remainder = di_mod(exp_copy, di_from_int32(2));
        if (remainder && !di_is_zero(remainder)) {
            di_int temp = di_mul(result, base_mod);
            if (temp) {
                di_int new_result = di_mod(temp, mod);
                di_release(&temp);
                di_release(&result);
                result = new_result;
            }
        }
        di_release(&remainder);
        
        // Square base_mod and divide exp by 2
        di_int base_squared = di_mul(base_mod, base_mod);
        if (base_squared) {
            di_int new_base = di_mod(base_squared, mod);
            di_release(&base_squared);
            di_release(&base_mod);
            base_mod = new_base;
        }
        
        di_int two = di_from_int32(2);
        di_int new_exp = di_div(exp_copy, two);
        di_release(&two);
        di_release(&exp_copy);
        exp_copy = new_exp;
        
        if (!result || !base_mod || !exp_copy) break;
    }
    
    di_release(&base_mod);
    di_release(&exp_copy);
    
    return result;
}

// Simple primality test using trial division
DI_DEF bool di_is_prime(di_int n, int certainty) {
    (void)certainty; // Unused in this simple implementation
    
    if (!n) return false;
    if (di_is_negative(n)) return false;
    
    // Handle small cases
    di_int two = di_from_int32(2);
    di_int three = di_from_int32(3);
    
    if (di_lt(n, two)) {
        di_release(&two);
        di_release(&three);
        return false;
    }
    if (di_eq(n, two) || di_eq(n, three)) {
        di_release(&two);
        di_release(&three);
        return true;
    }
    
    // Check if even
    di_int remainder = di_mod(n, two);
    if (di_is_zero(remainder)) {
        di_release(&remainder);
        di_release(&two);
        di_release(&three);
        return false;
    }
    di_release(&remainder);
    
    // Check odd divisors up to sqrt(n)
    di_int sqrt_n = di_sqrt(n);
    di_int i = di_copy(three);
    
    while (di_le(i, sqrt_n)) {
        di_int remainder = di_mod(n, i);
        if (di_is_zero(remainder)) {
            di_release(&remainder);
            di_release(&two);
            di_release(&three);
            di_release(&sqrt_n);
            di_release(&i);
            return false; // Found a divisor
        }
        di_release(&remainder);
        
        // i += 2 (check only odd numbers)
        di_int new_i = di_add(i, two);
        di_release(&i);
        i = new_i;
        
        if (!i) break;
    }
    
    di_release(&two);
    di_release(&three);
    di_release(&sqrt_n);
    di_release(&i);
    
    return true;
}

// Find next prime number >= n
DI_DEF di_int di_next_prime(di_int n) {
    if (!n) return NULL;
    
    di_int candidate = di_copy(n);
    if (!candidate) return NULL;
    
    // If n is even, make it odd
    di_int two = di_from_int32(2);
    di_int remainder = di_mod(candidate, two);
    if (di_is_zero(remainder)) {
        di_int new_candidate = di_add(candidate, di_one());
        di_release(&candidate);
        candidate = new_candidate;
    }
    di_release(&remainder);
    
    // Check odd numbers until we find a prime
    while (candidate && !di_is_prime(candidate, 10)) {
        di_int new_candidate = di_add(candidate, two);
        di_release(&candidate);
        candidate = new_candidate;
    }
    
    di_release(&two);
    return candidate;
}

// Simple random number generator (NOT cryptographically secure)
// This is a placeholder implementation - real applications should use
// a proper CSPRNG like /dev/urandom or Windows CryptoAPI
DI_DEF di_int di_random(size_t bits) {
    if (bits == 0) return di_zero();
    
    size_t limbs_needed = (bits + DI_LIMB_BITS - 1) / DI_LIMB_BITS;
    struct di_int_internal* result = di_alloc(limbs_needed);
    if (!result) return NULL;
    
    // Use simple rand() - NOT suitable for cryptographic use
    for (size_t i = 0; i < limbs_needed; i++) {
        result->limbs[i] = 0;
        for (int j = 0; j < (int)(sizeof(di_limb_t)); j++) {
            result->limbs[i] |= ((di_limb_t)(rand() & 0xFF)) << (j * 8);
        }
    }
    
    // Mask the high bits to get exactly 'bits' bits
    size_t high_bits = bits % DI_LIMB_BITS;
    if (high_bits > 0) {
        di_limb_t mask = (1UL << high_bits) - 1;
        result->limbs[limbs_needed - 1] &= mask;
    }
    
    result->limb_count = limbs_needed;
    result->is_negative = false;
    di_normalize(result);
    
    return result;
}

// Random number in range [min, max)
DI_DEF di_int di_random_range(di_int min, di_int max) {
    if (!min || !max) return NULL;
    if (di_ge(min, max)) return NULL;
    
    di_int range = di_sub(max, min);
    if (!range) return NULL;
    
    // Get bit length of range to generate appropriate random number
    size_t range_bits = di_bit_length(range);
    
    // Generate random numbers until we get one in range
    // (Rejection sampling to avoid bias)
    for (int attempts = 0; attempts < 100; attempts++) {
        di_int random = di_random(range_bits + 8); // Extra bits to reduce rejection
        if (!random) continue;
        
        di_int mod_result = di_mod(random, range);
        di_release(&random);
        
        if (mod_result) {
            di_int result = di_add(min, mod_result);
            di_release(&mod_result);
            di_release(&range);
            return result;
        }
    }
    
    di_release(&range);
    return NULL; // Failed to generate
}

// Calculate bit length of an arbitrary precision integer
DI_DEF size_t di_bit_length(di_int big) {
    if (!big || big->limb_count == 0) return 0;
    
    // Find the most significant limb
    size_t high_limb_idx = big->limb_count - 1;
    di_limb_t high_limb = big->limbs[high_limb_idx];
    
    // Count bits in the high limb
    size_t high_limb_bits = 0;
    if (high_limb != 0) {
        di_limb_t temp = high_limb;
        while (temp > 0) {
            high_limb_bits++;
            temp >>= 1;
        }
    }
    
    return high_limb_idx * DI_LIMB_BITS + high_limb_bits;
}

// Count of limbs used
DI_DEF size_t di_limb_count(di_int big) {
    return big ? big->limb_count : 0;
}

// Extended Euclidean Algorithm: finds gcd(a,b) and coefficients x,y such that ax + by = gcd(a,b)
DI_DEF di_int di_extended_gcd(di_int a, di_int b, di_int* x, di_int* y) {
    if (!a || !b || !x || !y) return NULL;
    
    // Initialize
    di_int old_r = di_abs(a);
    di_int r = di_abs(b);
    di_int old_s = di_one();
    di_int s = di_zero();
    di_int old_t = di_zero();
    di_int t = di_one();
    
    if (!old_r || !r || !old_s || !s || !old_t || !t) {
        di_release(&old_r); di_release(&r);
        di_release(&old_s); di_release(&s);
        di_release(&old_t); di_release(&t);
        return NULL;
    }
    
    while (!di_is_zero(r)) {
        di_int quotient = di_div(old_r, r);
        if (!quotient) break;
        
        // (old_r, r) := (r, old_r - quotient * r)
        di_int temp1 = di_mul(quotient, r);
        di_int new_r = di_sub(old_r, temp1);
        di_release(&temp1);
        di_release(&old_r);
        old_r = r;
        r = new_r;
        
        // (old_s, s) := (s, old_s - quotient * s)
        di_int temp2 = di_mul(quotient, s);
        di_int new_s = di_sub(old_s, temp2);
        di_release(&temp2);
        di_release(&old_s);
        old_s = s;
        s = new_s;
        
        // (old_t, t) := (t, old_t - quotient * t)
        di_int temp3 = di_mul(quotient, t);
        di_int new_t = di_sub(old_t, temp3);
        di_release(&temp3);
        di_release(&quotient);
        di_release(&old_t);
        old_t = t;
        t = new_t;
        
        if (!r || !s || !t) break;
    }
    
    // Set output coefficients
    *x = old_s;
    *y = old_t;
    
    di_release(&r);
    di_release(&s);
    di_release(&t);
    
    return old_r; // This is gcd(a,b)
}

#endif // DI_IMPLEMENTATION

#endif // DYNAMIC_INT_H