#include "builtins.h"
#include "datetime.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include dynamic_string for ds_builder functions (implementation in library_impl.c)
#include "/home/ed/CLionProjects/dynamic_string.h/dynamic_string.h"

// Include datetime for date/time functions
#include "datetime.h"

// Static random initialization flag
static int random_initialized = 0;

// Forward declaration for builtin_value_to_string
static value_t builtin_value_to_string(slate_vm* vm, int arg_count, value_t* args);

// Runtime error handling - exits with non-zero code for now, will throw exception later
void runtime_error(const char* message, ...) {
    va_list args;
    va_start(args, message);

    fprintf(stderr, "Runtime error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
    exit(1); // Non-zero exit code
}

// Register a built-in function in the VM's global namespace
void register_builtin(slate_vm* vm, const char* name, native_t func, int min_args, int max_args) {
    // Create a built-in function value
    value_t builtin_val = make_native((void*)func);

    // Store in the VM's global namespace
    do_set(vm->globals, name, &builtin_val, sizeof(value_t));
}

// Global String class storage
value_t* global_string_class = NULL;

// Global Array class storage
value_t* global_array_class = NULL;

// Global Range class storage  
value_t* global_range_class = NULL;

// Global Iterator class storage
value_t* global_iterator_class = NULL;

// Global StringBuilder class storage
value_t* global_string_builder_class = NULL;

// Global Buffer class storage
value_t* global_buffer_class = NULL;

// Global Int class storage
value_t* global_int_class = NULL;

// String factory function for creating strings from codepoints
static value_t string_factory(value_t* args, int arg_count) {
    // Create a string builder for efficient construction
    ds_builder sb = ds_builder_create();
    
    // Case 1: Single array argument containing codepoints
    if (arg_count == 1 && args[0].type == VAL_ARRAY) {
        da_array arr = args[0].as.array;
        size_t len = da_length(arr);
        
        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            
            // Each element must be an integer
            if (elem->type != VAL_INT32 && elem->type != VAL_BIGINT) {
                ds_builder_release(&sb);
                runtime_error("String() array elements must be integers (codepoints)");
                return make_null(); // Won't reach here due to exit in runtime_error
            }
            
            // Get the codepoint value
            uint32_t codepoint;
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0) {
                    ds_builder_release(&sb);
                    runtime_error("Codepoint cannot be negative: %d", elem->as.int32);
                    return make_null();
                }
                codepoint = (uint32_t)elem->as.int32;
            } else {
                // For bigint, convert to uint32_t and check range
                // For now, just get the low 32 bits (simplified)
                ds_builder_release(&sb);
                runtime_error("BigInt codepoints not yet supported");
                return make_null();
            }
            
            // Validate Unicode codepoint range
            if (codepoint > 0x10FFFF) {
                ds_builder_release(&sb);
                runtime_error("Invalid Unicode codepoint: 0x%X", codepoint);
                return make_null();
            }
            
            // Append the codepoint to the builder
            ds_builder_append_char(sb, codepoint);
        }
    }
    // Case 2: Multiple codepoint arguments (variadic)
    else {
        for (int i = 0; i < arg_count; i++) {
            // Each argument must be an integer
            if (args[i].type != VAL_INT32 && args[i].type != VAL_BIGINT) {
                ds_builder_release(&sb);
                runtime_error("String() arguments must be integers (codepoints)");
                return make_null();
            }
            
            // Get the codepoint value
            uint32_t codepoint;
            if (args[i].type == VAL_INT32) {
                if (args[i].as.int32 < 0) {
                    ds_builder_release(&sb);
                    runtime_error("Codepoint cannot be negative: %d", args[i].as.int32);
                    return make_null();
                }
                codepoint = (uint32_t)args[i].as.int32;
            } else {
                // For bigint, would need proper conversion
                ds_builder_release(&sb);
                runtime_error("BigInt codepoints not yet supported");
                return make_null();
            }
            
            // Validate Unicode codepoint range
            if (codepoint > 0x10FFFF) {
                ds_builder_release(&sb);
                runtime_error("Invalid Unicode codepoint: 0x%X", codepoint);
                return make_null();
            }
            
            // Append the codepoint to the builder
            ds_builder_append_char(sb, codepoint);
        }
    }
    
    // Convert builder to string
    ds_string result = ds_builder_to_string(sb);
    ds_builder_release(&sb);
    
    // Create and return the value
    return make_string_ds(result);
}

// Int factory function for converting strings to integers with optional base
static value_t int_factory(value_t* args, int arg_count) {
    if (arg_count == 0) {
        runtime_error("Int() requires at least 1 argument (the string to parse)");
        return make_null();
    }
    
    if (arg_count > 2) {
        runtime_error("Int() takes at most 2 arguments (string and optional base)");
        return make_null();
    }
    
    // First argument must be a string
    if (args[0].type != VAL_STRING) {
        runtime_error("Int() first argument must be a string");
        return make_null();
    }
    
    ds_string str = args[0].as.string;
    int base = 10; // Default base
    
    // If second argument provided, it must be the base
    if (arg_count == 2) {
        if (args[1].type != VAL_INT32) {
            runtime_error("Int() base argument must be an integer");
            return make_null();
        }
        base = args[1].as.int32;
        if (base < 2 || base > 36) {
            runtime_error("Int() base must be between 2 and 36, got %d", base);
            return make_null();
        }
    }
    
    // Parse the string
    char* endptr;
    long long result = strtoll(str, &endptr, base);
    
    // Check for parsing errors
    if (endptr == str) {
        runtime_error("Int() could not parse '%s' as integer in base %d", str, base);
        return make_null();
    }
    
    if (*endptr != '\0') {
        runtime_error("Int() found invalid characters in '%s' for base %d", str, base);
        return make_null();
    }
    
    // Check for overflow - if it fits in int32, use int32, otherwise use bigint
    if (result >= INT32_MIN && result <= INT32_MAX) {
        return make_int32((int32_t)result);
    } else {
        // Convert to bigint for large values
        di_int bigint_val = di_from_int64(result);
        return make_bigint(bigint_val);
    }
}

// Helper function: Safe integer multiplication with overflow promotion
static value_t safe_int_multiply(value_t a, value_t b) {
    // Handle int32 * int32 with overflow detection (same as VM)
    if (a.type == VAL_INT32 && b.type == VAL_INT32) {
        int32_t result;
        if (di_multiply_overflow_int32(a.as.int32, b.as.int32, &result)) {
            return make_int32(result);
        } else {
            // Overflow - promote to BigInt using the dynamic int library directly
            di_int a_big = di_from_int32(a.as.int32);
            di_int b_big = di_from_int32(b.as.int32);
            di_int big_result = di_mul(a_big, b_big);
            di_release(&a_big);
            di_release(&b_big);
            return make_bigint(big_result);
        }
    }
    // BigInt * BigInt
    else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
        di_int result = di_mul(a.as.bigint, b.as.bigint);
        return make_bigint(result);
    }
    // Mixed types: int32 * BigInt or BigInt * int32
    else {
        di_int a_big = (a.type == VAL_INT32) ? di_from_int32(a.as.int32) : a.as.bigint;
        di_int b_big = (b.type == VAL_INT32) ? di_from_int32(b.as.int32) : b.as.bigint;
        
        di_int result = di_mul(a_big, b_big);
        
        // Release temporary BigInts if we created them
        if (a.type == VAL_INT32) di_release(&a_big);
        if (b.type == VAL_INT32) di_release(&b_big);
        
        return make_bigint(result);
    }
}

// Int method: toString(base?) - Convert integer to string in specified base
static value_t builtin_int_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count < 1 || arg_count > 2) {
        runtime_error("toString() requires 0 or 1 arguments (optional base)");
        return make_null();
    }
    
    value_t receiver = args[0];
    int base = 10; // Default base
    
    if (arg_count == 2) {
        if (args[1].type != VAL_INT32) {
            runtime_error("toString() base argument must be an integer");
            return make_null();
        }
        base = args[1].as.int32;
        if (base < 2 || base > 36) {
            runtime_error("toString() base must be between 2 and 36, got %d", base);
            return make_null();
        }
    }
    
    // Handle both int32 and bigint
    if (receiver.type == VAL_INT32) {
        char buffer[64]; // Enough for any int32 in any base
        
        if (base == 10) {
            snprintf(buffer, sizeof(buffer), "%d", receiver.as.int32);
        } else {
            // Use stdlib function for other bases
            if (receiver.as.int32 >= 0) {
                snprintf(buffer, sizeof(buffer), "%*s", 0, ""); // Clear buffer
                char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
                int num = receiver.as.int32;
                int len = 0;
                
                if (num == 0) {
                    buffer[0] = '0';
                    buffer[1] = '\0';
                } else {
                    char temp[64];
                    while (num > 0) {
                        temp[len++] = digits[num % base];
                        num /= base;
                    }
                    // Reverse the string
                    for (int i = 0; i < len; i++) {
                        buffer[i] = temp[len - 1 - i];
                    }
                    buffer[len] = '\0';
                }
            } else {
                // Handle negative numbers
                int num = -receiver.as.int32;
                char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
                int len = 0;
                char temp[64];
                
                while (num > 0) {
                    temp[len++] = digits[num % base];
                    num /= base;
                }
                
                buffer[0] = '-';
                for (int i = 0; i < len; i++) {
                    buffer[i + 1] = temp[len - 1 - i];
                }
                buffer[len + 1] = '\0';
            }
        }
        
        return make_string(buffer);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, we'd need to implement base conversion
        // For now, only support base 10
        if (base != 10) {
            runtime_error("BigInt toString() currently only supports base 10");
            return make_null();
        }
        
        char* str = di_to_string(receiver.as.bigint, 10);
        value_t result = make_string(str);
        free(str); // di_to_string allocates memory that needs to be freed
        return result;
    } else {
        runtime_error("toString() can only be called on integers");
        return make_null();
    }
}

// Int method: setBit(position) - Set bit at position to 1
static value_t builtin_int_set_bit(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("setBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_arg = args[1];
    
    if (pos_arg.type != VAL_INT32) {
        runtime_error("setBit() position must be an integer");
        return make_null();
    }
    
    int position = pos_arg.as.int32;
    if (position < 0) {
        runtime_error("setBit() position must be non-negative, got %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error("setBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 | (1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, set the bit using bitwise operations
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted = di_shift_left(bit_value, position);
        di_int new_result = di_or(result, shifted);
        
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted);
        
        return make_bigint(new_result);
    } else {
        runtime_error("setBit() can only be called on integers");
        return make_null();
    }
}

// Int method: clearBit(position) - Set bit at position to 0
static value_t builtin_int_clear_bit(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("clearBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_arg = args[1];
    
    if (pos_arg.type != VAL_INT32) {
        runtime_error("clearBit() position must be an integer");
        return make_null();
    }
    
    int position = pos_arg.as.int32;
    if (position < 0) {
        runtime_error("clearBit() position must be non-negative, got %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error("clearBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 & ~(1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, clear the bit
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted = di_shift_left(bit_value, position);
        di_int mask = di_not(shifted);
        di_int new_result = di_and(result, mask);
        
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted);
        di_release(&mask);
        
        return make_bigint(new_result);
    } else {
        runtime_error("clearBit() can only be called on integers");
        return make_null();
    }
}

// Int method: toggleBit(position) - Flip bit at position
static value_t builtin_int_toggle_bit(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("toggleBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_arg = args[1];
    
    if (pos_arg.type != VAL_INT32) {
        runtime_error("toggleBit() position must be an integer");
        return make_null();
    }
    
    int position = pos_arg.as.int32;
    if (position < 0) {
        runtime_error("toggleBit() position must be non-negative, got %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error("toggleBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 ^ (1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, toggle the bit
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted = di_shift_left(bit_value, position);
        di_int new_result = di_xor(result, shifted);
        
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted);
        
        return make_bigint(new_result);
    } else {
        runtime_error("toggleBit() can only be called on integers");
        return make_null();
    }
}

// Int method: getBit(position) - Get bit at position (returns 0 or 1)
static value_t builtin_int_get_bit(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("getBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_arg = args[1];
    
    if (pos_arg.type != VAL_INT32) {
        runtime_error("getBit() position must be an integer");
        return make_null();
    }
    
    int position = pos_arg.as.int32;
    if (position < 0) {
        runtime_error("getBit() position must be non-negative, got %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            return make_int32(0); // Bits beyond range are 0 (for positive) or 1 (for negative sign extension)
        }
        int32_t bit = (receiver.as.int32 >> position) & 1;
        return make_int32(bit);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, get the bit
        di_int shifted = di_shift_right(receiver.as.bigint, position);
        di_int one = di_from_int32(1);
        di_int bit = di_and(shifted, one);
        int32_t result_val;
        di_to_int32(bit, &result_val);
        int result = (int)result_val;
        
        di_release(&shifted);
        di_release(&one);
        di_release(&bit);
        
        return make_int32(result);
    } else {
        runtime_error("getBit() can only be called on integers");
        return make_null();
    }
}

// Int method: countBits() - Count number of 1-bits (population count)
static value_t builtin_int_count_bits(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("countBits() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        uint32_t n = (uint32_t)receiver.as.int32;
        int count = 0;
        while (n) {
            count++;
            n &= n - 1; // Clear the lowest set bit
        }
        return make_int32(count);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, count bits - this is complex, for now return error
        runtime_error("countBits() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error("countBits() can only be called on integers");
        return make_null();
    }
}

// Int method: leadingZeros() - Count leading zero bits
static value_t builtin_int_leading_zeros(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("leadingZeros() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        if (receiver.as.int32 == 0) {
            return make_int32(32);
        }
        
        uint32_t n = (uint32_t)receiver.as.int32;
        int count = 0;
        if ((n >> 16) == 0) { count += 16; n <<= 16; }
        if ((n >> 24) == 0) { count += 8; n <<= 8; }
        if ((n >> 28) == 0) { count += 4; n <<= 4; }
        if ((n >> 30) == 0) { count += 2; n <<= 2; }
        if ((n >> 31) == 0) { count += 1; }
        
        return make_int32(count);
    } else if (receiver.type == VAL_BIGINT) {
        runtime_error("leadingZeros() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error("leadingZeros() can only be called on integers");
        return make_null();
    }
}

// Int method: trailingZeros() - Count trailing zero bits
static value_t builtin_int_trailing_zeros(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("trailingZeros() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        if (receiver.as.int32 == 0) {
            return make_int32(32);
        }
        
        uint32_t n = (uint32_t)receiver.as.int32;
        int count = 0;
        if ((n & 0xFFFF) == 0) { count += 16; n >>= 16; }
        if ((n & 0xFF) == 0) { count += 8; n >>= 8; }
        if ((n & 0xF) == 0) { count += 4; n >>= 4; }
        if ((n & 0x3) == 0) { count += 2; n >>= 2; }
        if ((n & 0x1) == 0) { count += 1; }
        
        return make_int32(count);
    } else if (receiver.type == VAL_BIGINT) {
        runtime_error("trailingZeros() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error("trailingZeros() can only be called on integers");
        return make_null();
    }
}

// Int method: isEven() - Check if integer is even
static value_t builtin_int_is_even(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEven() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        return make_boolean((receiver.as.int32 & 1) == 0);
    } else if (receiver.type == VAL_BIGINT) {
        di_int two = di_from_int32(2);
        di_int remainder = di_mod(receiver.as.bigint, two);
        di_int zero = di_from_int32(0);
        int is_even = di_eq(remainder, zero);
        di_release(&two);
        di_release(&remainder);
        di_release(&zero);
        return make_boolean(is_even);
    } else {
        runtime_error("isEven() can only be called on integers");
        return make_null();
    }
}

// Int method: isOdd() - Check if integer is odd
static value_t builtin_int_is_odd(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isOdd() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        return make_boolean((receiver.as.int32 & 1) == 1);
    } else if (receiver.type == VAL_BIGINT) {
        di_int two = di_from_int32(2);
        di_int remainder = di_mod(receiver.as.bigint, two);
        di_int one = di_from_int32(1);
        int is_odd = di_eq(remainder, one);
        di_release(&two);
        di_release(&remainder);
        di_release(&one);
        return make_boolean(is_odd);
    } else {
        runtime_error("isOdd() can only be called on integers");
        return make_null();
    }
}

// Int method: isPrime() - Primality test using trial division
static value_t builtin_int_is_prime(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isPrime() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        int32_t n = receiver.as.int32;
        
        if (n < 2) return make_boolean(false);
        if (n == 2) return make_boolean(true);
        if (n % 2 == 0) return make_boolean(false);
        
        // Check odd divisors up to sqrt(n)
        for (int32_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) {
                return make_boolean(false);
            }
        }
        return make_boolean(true);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, we'd need a more sophisticated primality test
        // For now, only handle small BigInts that fit in int32
        int32_t n;
        if (di_to_int32(receiver.as.bigint, &n)) {
            // Reuse int32 logic
            if (n < 2) return make_boolean(false);
            if (n == 2) return make_boolean(true);
            if (n % 2 == 0) return make_boolean(false);
            
            for (int32_t i = 3; i * i <= n; i += 2) {
                if (n % i == 0) {
                    return make_boolean(false);
                }
            }
            return make_boolean(true);
        } else {
            runtime_error("isPrime() not yet implemented for large BigInt values");
            return make_null();
        }
    } else {
        runtime_error("isPrime() can only be called on integers");
        return make_null();
    }
}

// Helper function to compute GCD using Euclidean algorithm
static int32_t compute_gcd(int32_t a, int32_t b) {
    a = a < 0 ? -a : a; // Make positive
    b = b < 0 ? -b : b; // Make positive
    
    while (b != 0) {
        int32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Int method: gcd(other) - Greatest common divisor
static value_t builtin_int_gcd(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("gcd() requires exactly 1 argument (other integer)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other_arg = args[1];
    
    if ((receiver.type != VAL_INT32 && receiver.type != VAL_BIGINT) ||
        (other_arg.type != VAL_INT32 && other_arg.type != VAL_BIGINT)) {
        runtime_error("gcd() arguments must be integers");
        return make_null();
    }
    
    // For simplicity, handle only int32 cases for now
    if (receiver.type == VAL_INT32 && other_arg.type == VAL_INT32) {
        int32_t result = compute_gcd(receiver.as.int32, other_arg.as.int32);
        return make_int32(result);
    } else {
        runtime_error("gcd() not yet implemented for BigInt");
        return make_null();
    }
}

// Int method: lcm(other) - Least common multiple
static value_t builtin_int_lcm(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("lcm() requires exactly 1 argument (other integer)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other_arg = args[1];
    
    if ((receiver.type != VAL_INT32 && receiver.type != VAL_BIGINT) ||
        (other_arg.type != VAL_INT32 && other_arg.type != VAL_BIGINT)) {
        runtime_error("lcm() arguments must be integers");
        return make_null();
    }
    
    // For simplicity, handle only int32 cases for now
    if (receiver.type == VAL_INT32 && other_arg.type == VAL_INT32) {
        int32_t a = receiver.as.int32;
        int32_t b = other_arg.as.int32;
        
        if (a == 0 || b == 0) {
            return make_int32(0);
        }
        
        int32_t gcd = compute_gcd(a, b);
        
        // LCM = |a * b| / GCD(a, b)
        // Be careful about overflow
        long long result = ((long long)abs(a) / gcd) * abs(b);
        
        if (result > INT32_MAX) {
            // Convert to BigInt if overflow
            di_int bigint_result = di_from_int64(result);
            return make_bigint(bigint_result);
        } else {
            return make_int32((int32_t)result);
        }
    } else {
        runtime_error("lcm() not yet implemented for BigInt");
        return make_null();
    }
}

// Int method: pow(exponent, modulus?) - Power with optional modular arithmetic
static value_t builtin_int_pow(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count < 2 || arg_count > 3) {
        runtime_error("pow() requires 1 or 2 arguments (exponent and optional modulus)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t exponent_arg = args[1];
    value_t modulus_arg = {0};
    bool has_modulus = (arg_count == 3);
    
    if (has_modulus) {
        modulus_arg = args[2];
    }
    
    // All arguments must be integers
    if ((receiver.type != VAL_INT32 && receiver.type != VAL_BIGINT) ||
        (exponent_arg.type != VAL_INT32 && exponent_arg.type != VAL_BIGINT) ||
        (has_modulus && modulus_arg.type != VAL_INT32 && modulus_arg.type != VAL_BIGINT)) {
        runtime_error("pow() arguments must be integers");
        return make_null();
    }
    
    // Handle int32 case with overflow checking
    if (receiver.type == VAL_INT32 && exponent_arg.type == VAL_INT32) {
        int32_t base = receiver.as.int32;
        int32_t exp = exponent_arg.as.int32;
        
        if (exp < 0) {
            runtime_error("pow() exponent must be non-negative, got %d", exp);
            return make_null();
        }
        
        if (has_modulus) {
            if (modulus_arg.type != VAL_INT32) {
                runtime_error("pow() modulus must be int32 when base and exponent are int32");
                return make_null();
            }
            int32_t mod = modulus_arg.as.int32;
            if (mod <= 0) {
                runtime_error("pow() modulus must be positive, got %d", mod);
                return make_null();
            }
            
            // Modular exponentiation using binary exponentiation
            long long result = 1;
            long long b = base % mod;
            
            while (exp > 0) {
                if (exp & 1) {
                    result = (result * b) % mod;
                }
                b = (b * b) % mod;
                exp >>= 1;
            }
            
            return make_int32((int32_t)result);
        } else {
            // Regular exponentiation with overflow detection
            if (exp == 0) return make_int32(1);
            if (base == 0) return make_int32(0);
            if (base == 1) return make_int32(1);
            if (base == -1) return make_int32(exp % 2 == 0 ? 1 : -1);
            
            // Use safe binary exponentiation with automatic overflow promotion
            value_t result_val = make_int32(1);
            value_t base_val = make_int32(base);
            int32_t remaining_exp = exp;
            
            // Binary exponentiation with overflow detection
            while (remaining_exp > 0) {
                if (remaining_exp & 1) {
                    // If exponent is odd, multiply result by current base
                    value_t temp = safe_int_multiply(result_val, base_val);
                    vm_release(result_val);
                    result_val = temp;
                }
                remaining_exp >>= 1;
                if (remaining_exp > 0) {
                    // Square the base for next iteration  
                    value_t temp = safe_int_multiply(base_val, base_val);
                    vm_release(base_val);
                    base_val = temp;
                }
            }
            
            vm_release(base_val);
            return result_val;
        }
    } else {
        runtime_error("pow() not yet implemented for BigInt");
        return make_null();
    }
}

// Int method: factorial() - Compute factorial with overflow handling
static value_t builtin_int_factorial(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("factorial() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_INT32 && receiver.type != VAL_BIGINT) {
        runtime_error("factorial() can only be called on integers");
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        int32_t n = receiver.as.int32;
        
        if (n < 0) {
            runtime_error("factorial() argument must be non-negative, got %d", n);
            return make_null();
        }
        
        if (n <= 1) {
            return make_int32(1);
        }
        
        // Use safe multiplication with automatic overflow promotion
        value_t result = make_int32(1);
        for (int32_t i = 2; i <= n; i++) {
            value_t factor = make_int32(i);
            value_t new_result = safe_int_multiply(result, factor);
            vm_release(result);
            vm_release(factor);
            result = new_result;
        }
        return result;
    } else {
        // BigInt factorial
        int32_t n;
        if (!di_to_int32(receiver.as.bigint, &n)) {
            runtime_error("factorial() not implemented for very large BigInt values");
            return make_null();
        }
        if (n < 0) {
            runtime_error("factorial() argument must be non-negative");
            return make_null();
        }
        
        value_t result = make_int32(1);
        for (int32_t i = 2; i <= n; i++) {
            value_t factor = make_int32(i);
            value_t new_result = safe_int_multiply(result, factor);
            vm_release(result);
            vm_release(factor);
            result = new_result;
        }
        return result;
    }
}

// StringBuilder method: append(string) - appends a string to the builder
static value_t builtin_string_builder_append(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("append() requires exactly 1 argument (the string to append)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t str_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("append() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert value to string if it's not already a string
    value_t string_val;
    if (str_val.type == VAL_STRING) {
        string_val = str_val;
    } else {
        // Call toString() on the value to convert it
        string_val = builtin_value_to_string(vm, 1, &str_val);
    }
    
    // Append the string to the builder
    ds_builder_append_string(receiver.as.string_builder, string_val.as.string);
    
    // Release the string if we created it
    if (str_val.type != VAL_STRING) {
        vm_release(string_val);
    }
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: appendChar(codepoint) - appends a Unicode codepoint
static value_t builtin_string_builder_append_char(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("appendChar() requires exactly 1 argument (the codepoint)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t codepoint_val = args[1];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("appendChar() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    if (codepoint_val.type != VAL_INT32) {
        runtime_error("appendChar() requires an integer codepoint, not %s", value_type_name(codepoint_val.type));
        return make_null();
    }
    
    uint32_t codepoint = (uint32_t)codepoint_val.as.int32;
    if (codepoint_val.as.int32 < 0 || codepoint > 0x10FFFF) {
        runtime_error("Invalid Unicode codepoint: 0x%X", codepoint);
        return make_null();
    }
    
    // Append the codepoint to the builder
    ds_builder_append_char(receiver.as.string_builder, codepoint);
    
    // Return the receiver for chaining
    return receiver;
}

// StringBuilder method: toString() - converts builder to string
static value_t builtin_string_builder_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toString() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("toString() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Convert builder to string (creates a copy)
    ds_string result = ds_builder_to_string(receiver.as.string_builder);
    return make_string_ds(result);
}

// StringBuilder method: length() - returns the current length
static value_t builtin_string_builder_length(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("length() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("length() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Get the current length of the builder
    size_t len = ds_builder_length(receiver.as.string_builder);
    return make_int32((int32_t)len);
}

// StringBuilder method: clear() - clears the builder
static value_t builtin_string_builder_clear(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("clear() requires no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_STRING_BUILDER) {
        runtime_error("clear() can only be called on StringBuilder, not %s", value_type_name(receiver.type));
        return make_null();
    }
    
    // Clear the builder
    ds_builder_clear(receiver.as.string_builder);
    
    // Return the receiver for chaining
    return receiver;
}

// LocalDate factory function
static value_t local_date_factory(value_t* args, int arg_count) {
    if (arg_count != 3) {
        runtime_error("LocalDate() requires 3 arguments: year, month, day");
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error("LocalDate() arguments must be numbers");
    }
    
    int year = (int)value_to_double(args[0]);
    int month = (int)value_to_double(args[1]);
    int day = (int)value_to_double(args[2]);
    
    // Validate date components
    if (!is_valid_date(year, month, day)) {
        runtime_error("Invalid date: %d-%02d-%02d", year, month, day);
    }
    
    local_date_t* date = local_date_create(NULL, year, month, day);
    assert(date != NULL); // Per user: allocation failures are assertion failures
    return make_local_date(date);
}

// LocalTime factory function
static value_t local_time_factory(value_t* args, int arg_count) {
    if (arg_count != 3 && arg_count != 4) {
        runtime_error("LocalTime() requires 3 or 4 arguments: hour, minute, second, [millisecond]");
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error("LocalTime() first 3 arguments must be numbers");
    }
    
    if (arg_count == 4 && !is_number(args[3])) {
        runtime_error("LocalTime() millisecond argument must be a number");
    }
    
    int hour = (int)value_to_double(args[0]);
    int minute = (int)value_to_double(args[1]);
    int second = (int)value_to_double(args[2]);
    int millis = (arg_count == 4) ? (int)value_to_double(args[3]) : 0;
    
    // Validate time components
    if (!is_valid_time(hour, minute, second, millis)) {
        runtime_error("Invalid time: %02d:%02d:%02d.%03d", hour, minute, second, millis);
    }
    
    local_time_t* time = local_time_create(NULL, hour, minute, second, millis);
    assert(time != NULL); // Per user: allocation failures are assertion failures
    return make_local_time(time);
}

// StringBuilder factory function
static value_t string_builder_factory(value_t* args, int arg_count) {
    // Parse optional initial capacity (first arg if it's an integer)
    size_t initial_capacity = 16; // default capacity
    int string_arg_start = 0;
    
    if (arg_count > 0 && (args[0].type == VAL_INT32 || args[0].type == VAL_BIGINT)) {
        // First argument is capacity
        if (args[0].type == VAL_INT32) {
            if (args[0].as.int32 < 0) {
                runtime_error("StringBuilder initial capacity cannot be negative: %d", args[0].as.int32);
                return make_null();
            }
            initial_capacity = (size_t)args[0].as.int32;
        } else {
            // For BigInt capacity, we'll just use default for simplicity
            runtime_error("BigInt capacity not yet supported for StringBuilder");
            return make_null();
        }
        string_arg_start = 1;
    }
    
    // Create builder with specified capacity
    ds_builder builder = ds_builder_create_with_capacity(initial_capacity);
    
    // Append any string arguments to the initial contents
    for (int i = string_arg_start; i < arg_count; i++) {
        if (args[i].type != VAL_STRING) {
            ds_builder_release(&builder);
            runtime_error("StringBuilder() string arguments must be strings, not %s", value_type_name(args[i].type));
            return make_null();
        }
        
        // Append the string to the builder
        ds_builder_append_string(builder, args[i].as.string);
    }
    
    // Create and return the StringBuilder value
    return make_string_builder(builder);
}

// Buffer factory function
static value_t buffer_factory(value_t* args, int arg_count) {
    if (arg_count == 0) {
        runtime_error("Buffer() requires at least 1 argument");
        return make_null();
    }

    // Single argument: string or array
    value_t arg = args[0];
    db_buffer buf;
    
    if (arg.type == VAL_STRING) {
        // Create buffer from string
        const char* str = arg.as.string;
        size_t len = str ? strlen(str) : 0;
        buf = db_new_with_data(str, len);
        return make_buffer(buf);
    } else if (arg.type == VAL_ARRAY) {
        // Create buffer from array of bytes (same logic as builtin_buffer)
        da_array arr = arg.as.array;
        size_t len = da_length(arr);

        // Convert array elements to bytes
        uint8_t* bytes = malloc(len);
        if (!bytes) {
            runtime_error("Failed to allocate memory for buffer");
            return make_null();
        }

        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            if (!elem) {
                free(bytes);
                runtime_error("Invalid array element at index %zu", i);
                return make_null();
            }
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0 || elem->as.int32 > 255) {
                    free(bytes);
                    runtime_error("Array element %d at index %zu is not a valid byte (0-255)", elem->as.int32, i);
                    return make_null();
                }
                bytes[i] = (uint8_t)elem->as.int32;
            } else {
                free(bytes);
                runtime_error("Array element at index %zu must be an integer, not %s", i, value_type_name(elem->type));
                return make_null();
            }
        }

        buf = db_new_with_data(bytes, len);
        free(bytes);
        return make_buffer(buf);
    } else {
        runtime_error("Buffer() argument must be a string or array, not %s", value_type_name(arg.type));
        return make_null();
    }
}

// ============================================================================
// Buffer instance methods (for prototypal inheritance)
// ============================================================================

// Buffer method: slice(offset, length)
// Returns a new buffer containing a slice of the original
value_t builtin_buffer_method_slice(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error("slice() takes exactly 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t offset_val = args[1];
    value_t length_val = args[2];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error("slice() can only be called on buffers");
    }
    if (offset_val.type != VAL_INT32) {
        runtime_error("slice() offset must be an integer, not %s", value_type_name(offset_val.type));
    }
    if (length_val.type != VAL_INT32) {
        runtime_error("slice() length must be an integer, not %s", value_type_name(length_val.type));
    }
    
    db_buffer buf = receiver.as.buffer;
    int32_t offset = offset_val.as.int32;
    int32_t length = length_val.as.int32;
    
    if (offset < 0 || length < 0) {
        runtime_error("slice() offset and length must be non-negative");
    }
    
    db_buffer slice = db_slice(buf, (size_t)offset, (size_t)length);
    if (!slice) {
        runtime_error("Invalid buffer slice bounds");
    }
    
    return make_buffer(slice);
}

// Buffer method: concat(other)
// Returns a new buffer with the contents of this buffer and the other buffer
value_t builtin_buffer_method_concat(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("concat() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other_val = args[1];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error("concat() can only be called on buffers");
    }
    if (other_val.type != VAL_BUFFER) {
        runtime_error("concat() argument must be a buffer, not %s", value_type_name(other_val.type));
    }
    
    db_buffer result = db_concat(receiver.as.buffer, other_val.as.buffer);
    return make_buffer(result);
}

// Buffer method: toHex()
// Returns a hex string representation of the buffer
value_t builtin_buffer_method_to_hex(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toHex() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error("toHex() can only be called on buffers");
    }
    
    db_buffer hex_buf = db_to_hex(receiver.as.buffer, false); // lowercase
    
    // Create null-terminated string from hex buffer
    size_t hex_len = db_size(hex_buf);
    char* null_term_hex = malloc(hex_len + 1);
    if (!null_term_hex) {
        db_release(&hex_buf);
        runtime_error("Failed to allocate memory for hex string");
    }
    
    memcpy(null_term_hex, hex_buf, hex_len);
    null_term_hex[hex_len] = '\0';
    
    ds_string hex_str = ds_new(null_term_hex);
    
    free(null_term_hex);
    db_release(&hex_buf);
    
    return make_string_ds(hex_str);
}

// Buffer method: length()
// Returns the length of the buffer in bytes
value_t builtin_buffer_method_length(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error("length() can only be called on buffers");
    }
    
    size_t size = db_size(receiver.as.buffer);
    return make_int32((int32_t)size);
}

// Buffer method: equals(other)
// Returns true if the buffer contents are equal to another buffer
value_t builtin_buffer_method_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other_val = args[1];
    
    if (receiver.type != VAL_BUFFER) {
        runtime_error("equals() can only be called on buffers");
    }
    if (other_val.type != VAL_BUFFER) {
        runtime_error("equals() argument must be a buffer, not %s", value_type_name(other_val.type));
    }
    
    bool equal = db_equals(receiver.as.buffer, other_val.as.buffer);
    return make_boolean(equal);
}

// Buffer method: toString()
// Returns the buffer contents as a string (assuming UTF-8 encoding)
value_t builtin_buffer_method_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toString() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error("toString() can only be called on buffers");
    }
    
    db_buffer buf = receiver.as.buffer;
    size_t size = db_size(buf);
    
    // Create null-terminated string from buffer
    char* null_term_str = malloc(size + 1);
    if (!null_term_str) {
        runtime_error("Failed to allocate memory for string");
    }
    
    memcpy(null_term_str, buf, size);
    null_term_str[size] = '\0';
    
    ds_string str = ds_new(null_term_str);
    
    free(null_term_str);
    
    return make_string_ds(str);
}

// Buffer method: reader()
// Returns a buffer reader for this buffer
value_t builtin_buffer_method_reader(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_BUFFER) {
        runtime_error("reader() can only be called on buffers");
    }
    
    db_reader reader = db_reader_new(receiver.as.buffer);
    return make_buffer_reader(reader);
}

// ============================================================================
// LocalDate instance methods
// ============================================================================

// LocalDate.year() - Get the year
value_t builtin_local_date_year(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.year() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.year() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_year(date));
}

// LocalDate.month() - Get the month
value_t builtin_local_date_month(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.month() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.month() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_month(date));
}

// LocalDate.day() - Get the day
value_t builtin_local_date_day(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.day() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.day() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day(date));
}

// LocalDate.dayOfWeek() - Get day of week (1=Monday, 7=Sunday)
value_t builtin_local_date_day_of_week(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfWeek() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfWeek() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_week(date));
}

// LocalDate.dayOfYear() - Get day of year (1-366)
value_t builtin_local_date_day_of_year(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfYear() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfYear() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_year(date));
}

// LocalDate.plusDays(days) - Add days to the date
value_t builtin_local_date_plus_days(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() days argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_days(NULL, date, days);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add days to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusMonths(months) - Add months to the date
value_t builtin_local_date_plus_months(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() months argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_months(NULL, date, months);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add months to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusYears(years) - Add years to the date
value_t builtin_local_date_plus_years(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() years argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_years(NULL, date, years);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add years to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusDays(days) - Subtract days from the date
value_t builtin_local_date_minus_days(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() days argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_days(NULL, date, -days);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract days from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusMonths(months) - Subtract months from the date
value_t builtin_local_date_minus_months(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() months argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_months(NULL, date, -months);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract months from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusYears(years) - Subtract years from the date
value_t builtin_local_date_minus_years(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() years argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_years(NULL, date, -years);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract years from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.equals(other) - Check if dates are equal
value_t builtin_local_date_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.equals() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.equals() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        return make_boolean(0);  // Different types are not equal
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_equals(date1, date2));
}

// LocalDate.isBefore(other) - Check if this date is before another
value_t builtin_local_date_is_before(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() other argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_before(date1, date2));
}

// LocalDate.isAfter(other) - Check if this date is after another
value_t builtin_local_date_is_after(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() other argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_after(date1, date2));
}

// LocalDate.toString() - Convert to string representation
value_t builtin_local_date_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.toString() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.toString() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    char* str = local_date_to_string(NULL, date);
    
    if (!str) {
        vm_runtime_error_with_debug(vm, "Failed to convert date to string");
        return make_null();
    }
    
    value_t result = make_string(str);
    free(str);  // make_string copies the string
    return result;
}

// ============================================================================
// LocalTime instance methods
// ============================================================================

// LocalTime.hour() - Get the hour
value_t builtin_local_time_hour(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.hour() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.hour() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_hour(time));
}

// LocalTime.minute() - Get the minute
value_t builtin_local_time_minute(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.minute() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minute() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_minute(time));
}

// LocalTime.second() - Get the second
value_t builtin_local_time_second(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.second() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.second() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_second(time));
}

// LocalTime.millisecond() - Get the millisecond
value_t builtin_local_time_millisecond(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.millisecond() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.millisecond() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_millisecond(time));
}

// LocalTime.plusHours(hours) - Add hours to the time
value_t builtin_local_time_plus_hours(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() takes 2 arguments (self, hours)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() hours argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int hours = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_hours(NULL, time, hours);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.plusMinutes(minutes) - Add minutes to the time
value_t builtin_local_time_plus_minutes(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() takes 2 arguments (self, minutes)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() minutes argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int minutes = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_minutes(NULL, time, minutes);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.plusSeconds(seconds) - Add seconds to the time
value_t builtin_local_time_plus_seconds(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() takes 2 arguments (self, seconds)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() seconds argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int seconds = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_seconds(NULL, time, seconds);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusHours(hours) - Subtract hours from the time
value_t builtin_local_time_minus_hours(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() takes 2 arguments (self, hours)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() hours argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int hours = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_hours(NULL, time, -hours);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusMinutes(minutes) - Subtract minutes from the time
value_t builtin_local_time_minus_minutes(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() takes 2 arguments (self, minutes)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() minutes argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int minutes = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_minutes(NULL, time, -minutes);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusSeconds(seconds) - Subtract seconds from the time
value_t builtin_local_time_minus_seconds(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() takes 2 arguments (self, seconds)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() seconds argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int seconds = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_seconds(NULL, time, -seconds);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.equals(other) - Check if times are equal
value_t builtin_local_time_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.equals() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.equals() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        return make_boolean(false);
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_equals(time1, time2));
}

// LocalTime.isBefore(other) - Check if this time is before another
value_t builtin_local_time_is_before(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() other argument must be a LocalTime");
        return make_null();
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_is_before(time1, time2));
}

// LocalTime.isAfter(other) - Check if this time is after another
value_t builtin_local_time_is_after(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() other argument must be a LocalTime");
        return make_null();
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_is_after(time1, time2));
}

// LocalTime.toString() - Convert to string representation
value_t builtin_local_time_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.toString() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.toString() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    char* str = local_time_to_string(NULL, time);
    
    assert(str != NULL);
    
    value_t result = make_string(str);
    free(str);  // make_string copies the string
    
    return result;
}

// Value.toString() - Universal toString method for all value types
static value_t builtin_value_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toString() takes no arguments (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_NULL:
            return make_string("null");
            
        case VAL_UNDEFINED:
            return make_string("undefined");
            
        case VAL_BOOLEAN:
            return make_string(receiver.as.boolean ? "true" : "false");
            
        case VAL_INT32:
            {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%d", receiver.as.int32);
                return make_string(buffer);
            }
            
        case VAL_BIGINT:
            {
                char* str = di_to_string(receiver.as.bigint, 10);
                value_t result = make_string(str);
                free(str);
                return result;
            }
            
        case VAL_NUMBER:
            {
                char buffer[64];
                // Use %.15g for clean formatting without unnecessary zeros
                snprintf(buffer, sizeof(buffer), "%.15g", receiver.as.number);
                return make_string(buffer);
            }
            
        case VAL_STRING:
            // Strings return themselves (no quotes)
            return make_string(receiver.as.string);
            
        case VAL_ARRAY:
            {
                // Format as "[1, 2, 3]"
                ds_builder sb = ds_builder_create();
                ds_string bracket_open = ds_new("[");
                ds_builder_append_string(sb, bracket_open);
                ds_release(&bracket_open);
                
                da_array array = receiver.as.array;
                size_t count = da_length(array);
                
                for (size_t i = 0; i < count; i++) {
                    if (i > 0) {
                        ds_string separator = ds_new(", ");
                        ds_builder_append_string(sb, separator);
                        ds_release(&separator);
                    }
                    
                    value_t* element = (value_t*)da_get(array, i);
                    // Recursively call toString on each element
                    value_t element_str = builtin_value_to_string(vm, 1, element);
                    ds_builder_append_string(sb, element_str.as.string);
                    vm_release(element_str);
                }
                
                ds_string bracket_close = ds_new("]");
                ds_builder_append_string(sb, bracket_close);
                ds_release(&bracket_close);
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_OBJECT:
            {
                // Format as "{key: value, key2: value2}"
                ds_builder sb = ds_builder_create();
                ds_string brace_open = ds_new("{");
                ds_builder_append_string(sb, brace_open);
                ds_release(&brace_open);
                
                // TODO: Implement object iteration when available
                // For now, return a placeholder
                ds_string placeholder = ds_new("...}");
                ds_builder_append_string(sb, placeholder);
                ds_release(&placeholder);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_STRING_BUILDER:
            {
                // Convert StringBuilder to string and format as "StringBuilder(...)"
                ds_string content = ds_builder_to_string(receiver.as.string_builder);
                ds_builder sb = ds_builder_create();
                ds_string prefix = ds_new("StringBuilder(\"");
                ds_builder_append_string(sb, prefix);
                ds_release(&prefix);
                ds_builder_append_string(sb, content);
                ds_string suffix = ds_new("\")");
                ds_builder_append_string(sb, suffix);
                ds_release(&suffix);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                ds_release(&content);
                
                return make_string_ds(result);
            }
            
        case VAL_LOCAL_DATE:
            {
                // Use existing LocalDate toString functionality
                return builtin_local_date_to_string(vm, arg_count, args);
            }
            
        case VAL_LOCAL_TIME:
            {
                // Use existing LocalTime toString functionality  
                return builtin_local_time_to_string(vm, arg_count, args);
            }
            
        case VAL_BUFFER:
            {
                // Convert buffer to hex representation
                return builtin_buffer_method_to_string(vm, arg_count, args);
            }
            
        case VAL_BUFFER_READER:
            return make_string("BufferReader");
            
        case VAL_RANGE:
            {
                // Format as "1..10" or "1..<10"
                range_t* range = receiver.as.range;
                ds_builder sb = ds_builder_create();
                
                // Convert start to string
                // Get the start value directly (already a value_t)
                value_t start_val = range->start;
                value_t start_str = builtin_value_to_string(vm, 1, &start_val);
                ds_builder_append_string(sb, start_str.as.string);
                vm_release(start_str);
                
                // Add range operator
                if (range->exclusive) {
                    ds_string range_op = ds_new("..<");
                    ds_builder_append_string(sb, range_op);
                    ds_release(&range_op);
                } else {
                    ds_string range_op = ds_new("..");
                    ds_builder_append_string(sb, range_op);
                    ds_release(&range_op);
                }
                
                // Convert end to string
                // Get the end value directly (already a value_t)  
                value_t end_val = range->end;
                value_t end_str = builtin_value_to_string(vm, 1, &end_val);
                ds_builder_append_string(sb, end_str.as.string);
                vm_release(end_str);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_ITERATOR:
            return make_string("Iterator");
            
        case VAL_NATIVE:
            return make_string("native function");
            
        case VAL_BOUND_METHOD:
            return make_string("bound method");
            
        default:
            return make_string("unknown");
    }
}

// Global Value class storage  
value_t* global_value_class = NULL;

// Initialize all built-in functions
void builtins_init(slate_vm* vm) {
    // Initialize random seed once
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }

    // Create the String class with its prototype
    do_object string_proto = do_create(NULL);

    // Add methods to String prototype
    value_t length_method = make_native(builtin_string_length);
    do_set(string_proto, "length", &length_method, sizeof(value_t));

    value_t substring_method = make_native(builtin_string_substring);
    do_set(string_proto, "substring", &substring_method, sizeof(value_t));

    value_t to_upper_method = make_native(builtin_string_to_upper);
    do_set(string_proto, "toUpper", &to_upper_method, sizeof(value_t));

    value_t to_lower_method = make_native(builtin_string_to_lower);
    do_set(string_proto, "toLower", &to_lower_method, sizeof(value_t));

    value_t trim_method = make_native(builtin_string_trim);
    do_set(string_proto, "trim", &trim_method, sizeof(value_t));

    value_t starts_with_method = make_native(builtin_string_starts_with);
    do_set(string_proto, "startsWith", &starts_with_method, sizeof(value_t));

    value_t ends_with_method = make_native(builtin_string_ends_with);
    do_set(string_proto, "endsWith", &ends_with_method, sizeof(value_t));

    value_t contains_method = make_native(builtin_string_contains);
    do_set(string_proto, "contains", &contains_method, sizeof(value_t));

    value_t replace_method = make_native(builtin_string_replace);
    do_set(string_proto, "replace", &replace_method, sizeof(value_t));

    value_t index_of_method = make_native(builtin_string_index_of);
    do_set(string_proto, "indexOf", &index_of_method, sizeof(value_t));

    value_t string_is_empty_method = make_native(builtin_string_is_empty);
    do_set(string_proto, "isEmpty", &string_is_empty_method, sizeof(value_t));

    value_t string_non_empty_method = make_native(builtin_string_non_empty);
    do_set(string_proto, "nonEmpty", &string_non_empty_method, sizeof(value_t));

    // Create the String class
    value_t string_class = make_class("String", string_proto);
    
    // Set the factory function to allow String(codepoint) or String([codepoints])
    string_class.as.class->factory = string_factory;

    // Store in globals
    do_set(vm->globals, "String", &string_class, sizeof(value_t));

    // Store a global reference for use in make_string
    static value_t string_class_storage;
    string_class_storage = vm_retain(string_class);
    global_string_class = &string_class_storage;

    // Create the Array class with its prototype
    do_object array_proto = do_create(NULL);

    // Add methods to Array prototype
    value_t array_length_method = make_native(builtin_array_length);
    do_set(array_proto, "length", &array_length_method, sizeof(value_t));

    value_t array_push_method = make_native(builtin_array_push);
    do_set(array_proto, "push", &array_push_method, sizeof(value_t));

    value_t array_pop_method = make_native(builtin_array_pop);
    do_set(array_proto, "pop", &array_pop_method, sizeof(value_t));

    value_t array_is_empty_method = make_native(builtin_array_is_empty);
    do_set(array_proto, "isEmpty", &array_is_empty_method, sizeof(value_t));

    value_t array_non_empty_method = make_native(builtin_array_non_empty);
    do_set(array_proto, "nonEmpty", &array_non_empty_method, sizeof(value_t));

    value_t array_index_of_method = make_native(builtin_array_index_of);
    do_set(array_proto, "indexOf", &array_index_of_method, sizeof(value_t));

    value_t array_contains_method = make_native(builtin_array_contains);
    do_set(array_proto, "contains", &array_contains_method, sizeof(value_t));

    value_t array_iterator_method = make_native(builtin_iterator);
    do_set(array_proto, "iterator", &array_iterator_method, sizeof(value_t));

    value_t array_copy_method = make_native(builtin_array_copy);
    do_set(array_proto, "copy", &array_copy_method, sizeof(value_t));

    value_t array_slice_method = make_native(builtin_array_slice);
    do_set(array_proto, "slice", &array_slice_method, sizeof(value_t));

    value_t array_reverse_method = make_native(builtin_array_reverse);
    do_set(array_proto, "reverse", &array_reverse_method, sizeof(value_t));

    // Create the Array class
    value_t array_class = make_class("Array", array_proto);

    // Store in globals
    do_set(vm->globals, "Array", &array_class, sizeof(value_t));

    // Store a global reference for use in make_array
    static value_t array_class_storage;
    array_class_storage = vm_retain(array_class);
    global_array_class = &array_class_storage;

    // Create the Range class with its prototype
    do_object range_proto = do_create(NULL);

    // Add methods to Range prototype
    value_t range_iterator_method = make_native(builtin_iterator);
    do_set(range_proto, "iterator", &range_iterator_method, sizeof(value_t));

    value_t range_start_method = make_native(builtin_range_start);
    do_set(range_proto, "start", &range_start_method, sizeof(value_t));

    value_t range_end_method = make_native(builtin_range_end);
    do_set(range_proto, "endValue", &range_end_method, sizeof(value_t));

    value_t range_is_exclusive_method = make_native(builtin_range_is_exclusive);
    do_set(range_proto, "isExclusive", &range_is_exclusive_method, sizeof(value_t));

    value_t range_is_empty_method = make_native(builtin_range_is_empty);
    do_set(range_proto, "isEmpty", &range_is_empty_method, sizeof(value_t));

    value_t range_length_method = make_native(builtin_range_length);
    do_set(range_proto, "length", &range_length_method, sizeof(value_t));

    value_t range_contains_method = make_native(builtin_range_contains);
    do_set(range_proto, "contains", &range_contains_method, sizeof(value_t));

    value_t range_to_array_method = make_native(builtin_range_to_array);
    do_set(range_proto, "toArray", &range_to_array_method, sizeof(value_t));

    value_t range_reverse_method = make_native(builtin_range_reverse);
    do_set(range_proto, "reverse", &range_reverse_method, sizeof(value_t));

    value_t range_equals_method = make_native(builtin_range_equals);
    do_set(range_proto, "equals", &range_equals_method, sizeof(value_t));

    // Create the Range class
    value_t range_class = make_class("Range", range_proto);

    // Store in globals
    do_set(vm->globals, "Range", &range_class, sizeof(value_t));

    // Store a global reference for use in make_range
    static value_t range_class_storage;
    range_class_storage = vm_retain(range_class);
    global_range_class = &range_class_storage;

    // Create the Iterator class with its prototype
    do_object iterator_proto = do_create(NULL);

    // Add methods to Iterator prototype
    value_t iterator_has_next_method = make_native(builtin_has_next);
    do_set(iterator_proto, "hasNext", &iterator_has_next_method, sizeof(value_t));

    value_t iterator_next_method = make_native(builtin_next);
    do_set(iterator_proto, "next", &iterator_next_method, sizeof(value_t));

    value_t iterator_is_empty_method = make_native(builtin_iterator_is_empty);
    do_set(iterator_proto, "isEmpty", &iterator_is_empty_method, sizeof(value_t));

    value_t iterator_to_array_method = make_native(builtin_iterator_to_array);
    do_set(iterator_proto, "toArray", &iterator_to_array_method, sizeof(value_t));

    // Create the Iterator class
    value_t iterator_class = make_class("Iterator", iterator_proto);

    // Store in globals
    do_set(vm->globals, "Iterator", &iterator_class, sizeof(value_t));

    // Store a global reference for use in make_iterator
    static value_t iterator_class_storage;
    iterator_class_storage = vm_retain(iterator_class);
    global_iterator_class = &iterator_class_storage;

    // Create the StringBuilder class with its prototype  
    do_object string_builder_proto = do_create(NULL);
    
    // Add methods to StringBuilder prototype
    value_t sb_append_method = make_native(builtin_string_builder_append);
    do_set(string_builder_proto, "append", &sb_append_method, sizeof(value_t));
    
    value_t sb_append_char_method = make_native(builtin_string_builder_append_char);
    do_set(string_builder_proto, "appendChar", &sb_append_char_method, sizeof(value_t));
    
    value_t sb_to_string_method = make_native(builtin_string_builder_to_string);
    do_set(string_builder_proto, "toString", &sb_to_string_method, sizeof(value_t));
    
    value_t sb_length_method = make_native(builtin_string_builder_length);
    do_set(string_builder_proto, "length", &sb_length_method, sizeof(value_t));
    
    value_t sb_clear_method = make_native(builtin_string_builder_clear);
    do_set(string_builder_proto, "clear", &sb_clear_method, sizeof(value_t));
    
    // Create the StringBuilder class
    value_t string_builder_class = make_class("StringBuilder", string_builder_proto);
    
    // Set the factory function 
    string_builder_class.as.class->factory = string_builder_factory;
    
    // Store in globals
    do_set(vm->globals, "StringBuilder", &string_builder_class, sizeof(value_t));
    
    // Store a global reference for use in make_string_builder
    static value_t string_builder_class_storage;
    string_builder_class_storage = vm_retain(string_builder_class);
    global_string_builder_class = &string_builder_class_storage;

    // Create the LocalDate class with its prototype
    do_object local_date_proto = do_create(NULL);

    // Add methods to LocalDate prototype
    value_t year_method = make_native(builtin_local_date_year);
    do_set(local_date_proto, "year", &year_method, sizeof(value_t));

    value_t month_method = make_native(builtin_local_date_month);
    do_set(local_date_proto, "month", &month_method, sizeof(value_t));

    value_t day_method = make_native(builtin_local_date_day);
    do_set(local_date_proto, "day", &day_method, sizeof(value_t));

    value_t day_of_week_method = make_native(builtin_local_date_day_of_week);
    do_set(local_date_proto, "dayOfWeek", &day_of_week_method, sizeof(value_t));

    value_t day_of_year_method = make_native(builtin_local_date_day_of_year);
    do_set(local_date_proto, "dayOfYear", &day_of_year_method, sizeof(value_t));

    value_t plus_days_method = make_native(builtin_local_date_plus_days);
    do_set(local_date_proto, "plusDays", &plus_days_method, sizeof(value_t));

    value_t plus_months_method = make_native(builtin_local_date_plus_months);
    do_set(local_date_proto, "plusMonths", &plus_months_method, sizeof(value_t));

    value_t plus_years_method = make_native(builtin_local_date_plus_years);
    do_set(local_date_proto, "plusYears", &plus_years_method, sizeof(value_t));

    value_t minus_days_method = make_native(builtin_local_date_minus_days);
    do_set(local_date_proto, "minusDays", &minus_days_method, sizeof(value_t));

    value_t minus_months_method = make_native(builtin_local_date_minus_months);
    do_set(local_date_proto, "minusMonths", &minus_months_method, sizeof(value_t));

    value_t minus_years_method = make_native(builtin_local_date_minus_years);
    do_set(local_date_proto, "minusYears", &minus_years_method, sizeof(value_t));

    value_t equals_method = make_native(builtin_local_date_equals);
    do_set(local_date_proto, "equals", &equals_method, sizeof(value_t));

    value_t is_before_method = make_native(builtin_local_date_is_before);
    do_set(local_date_proto, "isBefore", &is_before_method, sizeof(value_t));

    value_t is_after_method = make_native(builtin_local_date_is_after);
    do_set(local_date_proto, "isAfter", &is_after_method, sizeof(value_t));

    value_t to_string_method = make_native(builtin_local_date_to_string);
    do_set(local_date_proto, "toString", &to_string_method, sizeof(value_t));

    // Create the LocalDate class
    value_t local_date_class = make_class("LocalDate", local_date_proto);
    
    // Set the factory function to allow LocalDate(year, month, day)
    local_date_class.as.class->factory = local_date_factory;
    
    // Add static methods to the LocalDate class
    value_t now_method = make_native(builtin_local_date_now);
    do_set(local_date_class.as.class->properties, "now", &now_method, sizeof(value_t));
    
    value_t of_method = make_native(builtin_local_date_of);
    do_set(local_date_class.as.class->properties, "of", &of_method, sizeof(value_t));
    
    // Store in globals
    do_set(vm->globals, "LocalDate", &local_date_class, sizeof(value_t));

    // Store a global reference for use in make_local_date
    static value_t local_date_class_storage;
    local_date_class_storage = vm_retain(local_date_class);
    global_local_date_class = &local_date_class_storage;

    // Create the LocalTime class with its prototype
    do_object local_time_proto = do_create(NULL);

    // Add methods to LocalTime prototype
    value_t hour_method = make_native(builtin_local_time_hour);
    do_set(local_time_proto, "hour", &hour_method, sizeof(value_t));

    value_t minute_method = make_native(builtin_local_time_minute);
    do_set(local_time_proto, "minute", &minute_method, sizeof(value_t));

    value_t second_method = make_native(builtin_local_time_second);
    do_set(local_time_proto, "second", &second_method, sizeof(value_t));

    value_t millisecond_method = make_native(builtin_local_time_millisecond);
    do_set(local_time_proto, "millisecond", &millisecond_method, sizeof(value_t));

    value_t plus_hours_method = make_native(builtin_local_time_plus_hours);
    do_set(local_time_proto, "plusHours", &plus_hours_method, sizeof(value_t));

    value_t plus_minutes_method = make_native(builtin_local_time_plus_minutes);
    do_set(local_time_proto, "plusMinutes", &plus_minutes_method, sizeof(value_t));

    value_t plus_seconds_method = make_native(builtin_local_time_plus_seconds);
    do_set(local_time_proto, "plusSeconds", &plus_seconds_method, sizeof(value_t));

    value_t minus_hours_method = make_native(builtin_local_time_minus_hours);
    do_set(local_time_proto, "minusHours", &minus_hours_method, sizeof(value_t));

    value_t minus_minutes_method = make_native(builtin_local_time_minus_minutes);
    do_set(local_time_proto, "minusMinutes", &minus_minutes_method, sizeof(value_t));

    value_t minus_seconds_method = make_native(builtin_local_time_minus_seconds);
    do_set(local_time_proto, "minusSeconds", &minus_seconds_method, sizeof(value_t));

    value_t time_equals_method = make_native(builtin_local_time_equals);
    do_set(local_time_proto, "equals", &time_equals_method, sizeof(value_t));

    value_t time_is_before_method = make_native(builtin_local_time_is_before);
    do_set(local_time_proto, "isBefore", &time_is_before_method, sizeof(value_t));

    value_t time_is_after_method = make_native(builtin_local_time_is_after);
    do_set(local_time_proto, "isAfter", &time_is_after_method, sizeof(value_t));

    value_t time_to_string_method = make_native(builtin_local_time_to_string);
    do_set(local_time_proto, "toString", &time_to_string_method, sizeof(value_t));

    // Create the LocalTime class
    value_t local_time_class = make_class("LocalTime", local_time_proto);
    
    // Set the factory function to allow LocalTime(hour, minute, second, [millis])
    local_time_class.as.class->factory = local_time_factory;
    
    // Store in globals
    do_set(vm->globals, "LocalTime", &local_time_class, sizeof(value_t));

    // Store a global reference for use in make_local_time
    static value_t local_time_class_storage;
    local_time_class_storage = vm_retain(local_time_class);
    global_local_time_class = &local_time_class_storage;

    // Create the Buffer class with its prototype
    do_object buffer_proto = do_create(NULL);

    // Add methods to Buffer prototype
    value_t buffer_slice_method = make_native(builtin_buffer_method_slice);
    do_set(buffer_proto, "slice", &buffer_slice_method, sizeof(value_t));

    value_t buffer_concat_method = make_native(builtin_buffer_method_concat);
    do_set(buffer_proto, "concat", &buffer_concat_method, sizeof(value_t));

    value_t buffer_to_hex_method = make_native(builtin_buffer_method_to_hex);
    do_set(buffer_proto, "toHex", &buffer_to_hex_method, sizeof(value_t));

    value_t buffer_length_method = make_native(builtin_buffer_method_length);
    do_set(buffer_proto, "length", &buffer_length_method, sizeof(value_t));

    value_t buffer_equals_method = make_native(builtin_buffer_method_equals);
    do_set(buffer_proto, "equals", &buffer_equals_method, sizeof(value_t));

    value_t buffer_to_string_method = make_native(builtin_buffer_method_to_string);
    do_set(buffer_proto, "toString", &buffer_to_string_method, sizeof(value_t));

    value_t buffer_reader_method = make_native(builtin_buffer_method_reader);
    do_set(buffer_proto, "reader", &buffer_reader_method, sizeof(value_t));

    // Create the Buffer class
    value_t buffer_class = make_class("Buffer", buffer_proto);

    // Set the factory function 
    buffer_class.as.class->factory = buffer_factory;

    // Add static methods to the Buffer class
    value_t from_hex_method = make_native(builtin_buffer_from_hex);
    do_set(buffer_class.as.class->properties, "fromHex", &from_hex_method, sizeof(value_t));

    // Store in globals
    do_set(vm->globals, "Buffer", &buffer_class, sizeof(value_t));

    // Store a global reference for use in make_buffer
    static value_t buffer_class_storage;
    buffer_class_storage = vm_retain(buffer_class);
    global_buffer_class = &buffer_class_storage;

    // Register all built-ins
    register_builtin(vm, "print", builtin_print, 1, 1);
    register_builtin(vm, "type", builtin_type, 1, 1);
    register_builtin(vm, "abs", builtin_abs, 1, 1);
    register_builtin(vm, "sqrt", builtin_sqrt, 1, 1);
    register_builtin(vm, "floor", builtin_floor, 1, 1);
    register_builtin(vm, "ceil", builtin_ceil, 1, 1);
    register_builtin(vm, "round", builtin_round, 1, 1);
    register_builtin(vm, "min", builtin_min, 2, 2);
    register_builtin(vm, "max", builtin_max, 2, 2);
    register_builtin(vm, "random", builtin_random, 0, 0);
    register_builtin(vm, "sin", builtin_sin, 1, 1);
    register_builtin(vm, "cos", builtin_cos, 1, 1);
    register_builtin(vm, "tan", builtin_tan, 1, 1);
    register_builtin(vm, "asin", builtin_asin, 1, 1);
    register_builtin(vm, "acos", builtin_acos, 1, 1);
    register_builtin(vm, "atan", builtin_atan, 1, 1);
    register_builtin(vm, "atan2", builtin_atan2, 2, 2);
    register_builtin(vm, "degrees", builtin_degrees, 1, 1);
    register_builtin(vm, "radians", builtin_radians, 1, 1);
    register_builtin(vm, "exp", builtin_exp, 1, 1);
    register_builtin(vm, "ln", builtin_ln, 1, 1);
    register_builtin(vm, "sign", builtin_sign, 1, 1);
    register_builtin(vm, "input", builtin_input, 0, 1);
    register_builtin(vm, "parse_int", builtin_parse_int, 1, 1);
    register_builtin(vm, "parse_number", builtin_parse_number, 1, 1);
    register_builtin(vm, "args", builtin_args, 0, 0);

    // Iterator functions
    register_builtin(vm, "iterator", builtin_iterator, 1, 1);
    register_builtin(vm, "hasNext", builtin_has_next, 1, 1);
    register_builtin(vm, "next", builtin_next, 1, 1);

    // Buffer functions
    register_builtin(vm, "buffer", builtin_buffer, 1, 1);
    register_builtin(vm, "buffer_from_hex", builtin_buffer_from_hex, 1, 1);
    register_builtin(vm, "buffer_slice", builtin_buffer_slice, 3, 3);
    register_builtin(vm, "buffer_concat", builtin_buffer_concat, 2, 2);
    register_builtin(vm, "buffer_to_hex", builtin_buffer_to_hex, 1, 1);

    // Buffer builder functions
    register_builtin(vm, "buffer_builder", builtin_buffer_builder, 1, 1);
    register_builtin(vm, "builder_append_uint8", builtin_builder_append_uint8, 2, 2);
    register_builtin(vm, "builder_append_uint16_le", builtin_builder_append_uint16_le, 2, 2);
    register_builtin(vm, "builder_append_uint32_le", builtin_builder_append_uint32_le, 2, 2);
    register_builtin(vm, "builder_append_cstring", builtin_builder_append_cstring, 2, 2);
    register_builtin(vm, "builder_finish", builtin_builder_finish, 1, 1);

    // Buffer reader functions
    register_builtin(vm, "buffer_reader", builtin_buffer_reader, 1, 1);
    register_builtin(vm, "reader_read_uint8", builtin_reader_read_uint8, 1, 1);
    register_builtin(vm, "reader_read_uint16_le", builtin_reader_read_uint16_le, 1, 1);
    register_builtin(vm, "reader_read_uint32_le", builtin_reader_read_uint32_le, 1, 1);
    register_builtin(vm, "reader_position", builtin_reader_position, 1, 1);
    register_builtin(vm, "reader_remaining", builtin_reader_remaining, 1, 1);

    // I/O functions
    register_builtin(vm, "read_file", builtin_read_file, 1, 1);
    register_builtin(vm, "write_file", builtin_write_file, 2, 2);
    
    // LocalDate static functions
    register_builtin(vm, "LocalDate_now", builtin_local_date_now, 0, 0);
    register_builtin(vm, "LocalDate_of", builtin_local_date_of, 3, 3);
    
    // Create the Int class with its prototype
    do_object int_proto = do_create(NULL);
    
    // Add methods to Int prototype
    value_t int_to_string_method = make_native(builtin_int_to_string);
    do_set(int_proto, "toString", &int_to_string_method, sizeof(value_t));
    
    value_t set_bit_method = make_native(builtin_int_set_bit);
    do_set(int_proto, "setBit", &set_bit_method, sizeof(value_t));
    
    value_t clear_bit_method = make_native(builtin_int_clear_bit);
    do_set(int_proto, "clearBit", &clear_bit_method, sizeof(value_t));
    
    value_t toggle_bit_method = make_native(builtin_int_toggle_bit);
    do_set(int_proto, "toggleBit", &toggle_bit_method, sizeof(value_t));
    
    value_t get_bit_method = make_native(builtin_int_get_bit);
    do_set(int_proto, "getBit", &get_bit_method, sizeof(value_t));
    
    value_t count_bits_method = make_native(builtin_int_count_bits);
    do_set(int_proto, "countBits", &count_bits_method, sizeof(value_t));
    
    value_t leading_zeros_method = make_native(builtin_int_leading_zeros);
    do_set(int_proto, "leadingZeros", &leading_zeros_method, sizeof(value_t));
    
    value_t trailing_zeros_method = make_native(builtin_int_trailing_zeros);
    do_set(int_proto, "trailingZeros", &trailing_zeros_method, sizeof(value_t));
    
    value_t is_even_method = make_native(builtin_int_is_even);
    do_set(int_proto, "isEven", &is_even_method, sizeof(value_t));
    
    value_t is_odd_method = make_native(builtin_int_is_odd);
    do_set(int_proto, "isOdd", &is_odd_method, sizeof(value_t));
    
    value_t is_prime_method = make_native(builtin_int_is_prime);
    do_set(int_proto, "isPrime", &is_prime_method, sizeof(value_t));
    
    value_t gcd_method = make_native(builtin_int_gcd);
    do_set(int_proto, "gcd", &gcd_method, sizeof(value_t));
    
    value_t lcm_method = make_native(builtin_int_lcm);
    do_set(int_proto, "lcm", &lcm_method, sizeof(value_t));
    
    value_t pow_method = make_native(builtin_int_pow);
    do_set(int_proto, "pow", &pow_method, sizeof(value_t));
    
    value_t factorial_method = make_native(builtin_int_factorial);
    do_set(int_proto, "factorial", &factorial_method, sizeof(value_t));
    
    // Create the Int class
    value_t int_class = make_class("Int", int_proto);
    
    // Set the factory function to allow Int(string, base?)
    int_class.as.class->factory = int_factory;
    
    // Store in globals
    do_set(vm->globals, "Int", &int_class, sizeof(value_t));
    
    // Store a global reference for use in make_int32 and make_bigint
    static value_t int_class_storage;
    int_class_storage = vm_retain(int_class);
    global_int_class = &int_class_storage;

    // Create the Value class - the ultimate superclass of all values
    do_object value_proto = do_create(NULL);
    
    // Add methods to Value prototype
    value_t value_to_string_method = make_native(builtin_value_to_string);
    do_set(value_proto, "toString", &value_to_string_method, sizeof(value_t));
    
    // Create the Value class
    value_t value_class = make_class("Value", value_proto);
    
    // Value class has no factory (factory = NULL) - cannot be instantiated directly
    value_class.as.class->factory = NULL;
    
    // Value class should have no parent class (it's the root class)
    value_class.class = NULL;
    
    // Store in globals (though it shouldn't be called directly)
    do_set(vm->globals, "Value", &value_class, sizeof(value_t));
    
    // Store a global reference for use in make_value functions
    static value_t value_class_storage;
    value_class_storage = vm_retain(value_class);
    global_value_class = &value_class_storage;
    
    // Now that Value class is created, set up proper inheritance chain
    // Array class should inherit from Value class
    if (global_array_class && global_array_class->type == VAL_CLASS) {
        global_array_class->class = &value_class_storage;
    }
    
    // Int class should inherit from Value class
    if (global_int_class && global_int_class->type == VAL_CLASS) {
        global_int_class->class = &value_class_storage;
    }
}

// Built-in function implementations

// print(value) - Print any value to console
value_t builtin_print(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("print() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Use the specialized print function for builtins
    print_for_builtin(vm, arg);
    printf("\n");

    return make_undefined(); // print returns void
}

// type(value) - Get type name as string
value_t builtin_type(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("type() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    const char* type_name;

    switch (arg.type) {
    case VAL_INT32:
        type_name = "int32";
        break;
    case VAL_BIGINT:
        type_name = "bigint";
        break;
    case VAL_NUMBER:
        type_name = "number";
        break;
    case VAL_STRING:
        type_name = "string";
        break;
    case VAL_BOOLEAN:
        type_name = "boolean";
        break;
    case VAL_NULL:
        type_name = "null";
        break;
    case VAL_UNDEFINED:
        type_name = "undefined";
        break;
    case VAL_ARRAY:
        type_name = "array";
        break;
    case VAL_OBJECT:
        type_name = "object";
        break;
    case VAL_CLASS:
        type_name = "class";
        break;
    case VAL_FUNCTION:
        type_name = "function";
        break;
    case VAL_CLOSURE:
        type_name = "closure";
        break;
    case VAL_NATIVE:
        type_name = "builtin";
        break;
    case VAL_RANGE:
        type_name = "range";
        break;
    case VAL_ITERATOR:
        type_name = "iterator";
        break;
    case VAL_BUFFER:
        type_name = "buffer";
        break;
    case VAL_BUFFER_BUILDER:
        type_name = "buffer_builder";
        break;
    case VAL_BUFFER_READER:
        type_name = "buffer_reader";
        break;
    case VAL_BOUND_METHOD:
        type_name = "bound_method";
        break;
    case VAL_LOCAL_DATE:
        type_name = "LocalDate";
        break;
    case VAL_LOCAL_TIME:
        type_name = "LocalTime";
        break;
    case VAL_LOCAL_DATETIME:
        type_name = "LocalDateTime";
        break;
    case VAL_ZONED_DATETIME:
        type_name = "ZonedDateTime";
        break;
    case VAL_INSTANT:
        type_name = "Instant";
        break;
    case VAL_DURATION:
        type_name = "Duration";
        break;
    case VAL_PERIOD:
        type_name = "Period";
        break;
    default:
        type_name = "unknown";
        break;
    }

    return make_string(type_name);
}

// abs(number) - Absolute value
value_t builtin_abs(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("abs() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        int32_t val = arg.as.int32;
        if (val == INT32_MIN) {
            // abs(INT32_MIN) overflows to BigInt
            di_int big = di_from_int64(-(int64_t)INT32_MIN);
            return make_bigint(big);
        } else {
            return make_int32(val < 0 ? -val : val);
        }
    } else if (arg.type == VAL_BIGINT) {
        di_int result = di_abs(arg.as.bigint);
        return make_bigint(result);
    } else if (arg.type == VAL_NUMBER) {
        return make_number(fabs(arg.as.number));
    } else {
        runtime_error("abs() requires a number argument");
    }
}

// sqrt(number) - Square root
value_t builtin_sqrt(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sqrt() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sqrt() requires a number argument");
    }

    double val = value_to_double(arg);

    if (val < 0) {
        runtime_error("sqrt() of negative number");
    }

    return make_number(sqrt(val));
}

// floor(number) - Floor function
value_t builtin_floor(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("floor() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "floored"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "floored"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = floor(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("floor() requires a number argument");
    }
}

// ceil(number) - Ceiling function
value_t builtin_ceil(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("ceil() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "ceiled"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "ceiled"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = ceil(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("ceil() requires a number argument");
    }
}

// round(number) - Round to nearest integer
value_t builtin_round(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("round() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Handle all numeric types
    if (arg.type == VAL_INT32) {
        // Integers are already "rounded"
        return arg;
    } else if (arg.type == VAL_BIGINT) {
        // BigInts are already "rounded"
        return arg;
    } else if (arg.type == VAL_NUMBER) {
        double result = round(arg.as.number);
        // Try to return as int32 if it fits
        if (result >= INT32_MIN && result <= INT32_MAX && result == (int32_t)result) {
            return make_int32((int32_t)result);
        } else {
            return make_number(result);
        }
    } else {
        runtime_error("round() requires a number argument");
    }
}

// min(a, b) - Minimum of two values
value_t builtin_min(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("min() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error("min() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_double(a);
    double b_val = value_to_double(b);

    // Return the smaller value with its original type
    return (a_val < b_val) ? a : b;
}

// max(a, b) - Maximum of two values
value_t builtin_max(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("max() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t a = args[0];
    value_t b = args[1];

    // Check if both are numeric types
    if (!is_number(a) || !is_number(b)) {
        runtime_error("max() requires number arguments");
    }

    // Convert both to double for comparison
    double a_val = value_to_double(a);
    double b_val = value_to_double(b);

    // Return the larger value with its original type
    return (a_val > b_val) ? a : b;
}

// random() - Random number 0.0 to 1.0
value_t builtin_random(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error("random() takes no arguments (%d given)", arg_count);
    }

    return make_number((double)rand() / RAND_MAX);
}

// sin(number) - Sine function (radians)
value_t builtin_sin(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sin() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(sin(val));
}

// cos(number) - Cosine function (radians)
value_t builtin_cos(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("cos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("cos() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(cos(val));
}

// tan(number) - Tangent function (radians)
value_t builtin_tan(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("tan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("tan() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(tan(val));
}

// exp(number) - Exponential function (e^x)
value_t builtin_exp(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("exp() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("exp() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(exp(val));
}

// ln(number) - Natural logarithm (base e)
value_t builtin_ln(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("ln() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("ln() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (ln of non-positive numbers)
    if (val <= 0) {
        runtime_error("ln() domain error: argument must be positive");
    }

    return make_number(log(val));
}

// asin(number) - Inverse sine function (returns radians)
value_t builtin_asin(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("asin() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("asin() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (asin domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error("asin() domain error: argument must be in range [-1, 1]");
    }

    return make_number(asin(val));
}

// acos(number) - Inverse cosine function (returns radians)
value_t builtin_acos(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("acos() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("acos() requires a number argument");
    }

    double val = value_to_double(arg);

    // Check for domain error (acos domain is [-1, 1])
    if (val < -1.0 || val > 1.0) {
        runtime_error("acos() domain error: argument must be in range [-1, 1]");
    }

    return make_number(acos(val));
}

// atan(number) - Inverse tangent function (returns radians)
value_t builtin_atan(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("atan() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("atan() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(atan(val));
}

// atan2(y, x) - Two-argument inverse tangent function (returns radians)
value_t builtin_atan2(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("atan2() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t y = args[0];
    value_t x = args[1];

    // Check if both are numeric types
    if (!is_number(y) || !is_number(x)) {
        runtime_error("atan2() requires number arguments");
    }

    double y_val = value_to_double(y);
    double x_val = value_to_double(x);

    return make_number(atan2(y_val, x_val));
}

// degrees(radians) - Convert radians to degrees
value_t builtin_degrees(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("degrees() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("degrees() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(val * 180.0 / M_PI);
}

// radians(degrees) - Convert degrees to radians
value_t builtin_radians(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("radians() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("radians() requires a number argument");
    }

    double val = value_to_double(arg);

    return make_number(val * M_PI / 180.0);
}

// sign(number) - Sign function (-1, 0, or 1)
value_t builtin_sign(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("sign() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];

    // Check if it's a numeric type
    if (!is_number(arg)) {
        runtime_error("sign() requires a number argument");
    }

    double val = value_to_double(arg);

    if (val > 0.0) {
        return make_int32(1);
    } else if (val < 0.0) {
        return make_int32(-1);
    } else {
        return make_int32(0);
    }
}

// input(prompt) - Read user input with optional prompt
value_t builtin_input(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count > 1) {
        runtime_error("input() takes 0 or 1 arguments (%d given)", arg_count);
    }

    // Print prompt if provided
    if (arg_count == 1) {
        value_t prompt = args[0];
        if (prompt.type == VAL_STRING) {
            printf("%s", prompt.as.string);
            fflush(stdout);
        } else {
            runtime_error("input() prompt must be a string");
        }
    }

    // Read line from stdin
    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);

    if (read == -1) {
        // EOF or error
        free(line);
        return make_null();
    }

    // Remove trailing newline if present
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    // Create string value and free the buffer
    ds_string result = ds_new(line);
    free(line);

    return make_string_ds(result);
}

// parse_int(string) - Convert string to integer
value_t builtin_parse_int(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("parse_int() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error("parse_int() requires a string argument");
    }

    char* endptr;
    const char* str = arg.as.string;

    // Try parsing as long long first
    long long val = strtoll(str, &endptr, 10);

    // Check if entire string was consumed
    if (*endptr != '\0') {
        runtime_error("'%s' is not a valid integer", str);
    }

    // Check if it fits in int32
    if (val >= INT32_MIN && val <= INT32_MAX) {
        return make_int32((int32_t)val);
    } else {
        // Use BigInt for large numbers
        di_int big = di_from_int64(val);
        return make_bigint(big);
    }
}

// parse_number(string) - Convert string to number (int or float)
value_t builtin_parse_number(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("parse_number() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    if (arg.type != VAL_STRING) {
        runtime_error("parse_number() requires a string argument");
    }

    const char* str = arg.as.string;
    char* endptr;

    // Check if it contains a decimal point
    if (strchr(str, '.') != NULL) {
        // Parse as float
        double val = strtod(str, &endptr);

        if (*endptr != '\0') {
            runtime_error("'%s' is not a valid number", str);
        }

        return make_number(val);
    } else {
        // Parse as integer first, then convert to appropriate type
        long long val = strtoll(str, &endptr, 10);

        if (*endptr != '\0') {
            runtime_error("'%s' is not a valid number", str);
        }

        // Check if it fits in int32
        if (val >= INT32_MIN && val <= INT32_MAX) {
            return make_int32((int32_t)val);
        } else {
            // Use BigInt for large numbers
            di_int big = di_from_int64(val);
            return make_bigint(big);
        }
    }
}

// args() - Get command line arguments as array
value_t builtin_args(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error("args() takes no arguments (%d given)", arg_count);
    }

    // Create array of command line arguments
    da_array arg_array = da_new(sizeof(value_t));

    for (int i = 0; i < vm->argc; i++) {
        value_t arg_val = make_string(vm->argv[i]);
        da_push(arg_array, &arg_val);
    }

    return make_array(arg_array);
}

// String method: length
// This will be used as a method on the String prototype
value_t builtin_string_length(slate_vm* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the string)
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("length() can only be called on strings");
    }

    // Get the length of the string
    size_t length = ds_length(receiver.as.string);

    // Return as int32 if it fits, otherwise as number
    if (length <= INT32_MAX) {
        return make_int32((int32_t)length);
    } else {
        return make_number((double)length);
    }
}

// String method: substring(start, length)
value_t builtin_string_substring(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) { // receiver + 2 args
        runtime_error("substring() takes exactly 2 arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t start_val = args[1];
    value_t length_val = args[2];

    if (receiver.type != VAL_STRING) {
        runtime_error("substring() can only be called on strings");
    }
    if (!is_int(start_val) || !is_int(length_val)) {
        runtime_error("substring() arguments must be integers");
    }

    int start_int = value_to_int(start_val);
    int length_int = value_to_int(length_val);
    
    if (start_int < 0 || length_int < 0) {
        runtime_error("substring() arguments must be non-negative");
    }
    
    size_t start = (size_t)start_int;
    size_t length = (size_t)length_int;

    ds_string result = ds_substring(receiver.as.string, start, length);
    return make_string_ds(result);
}

// String method: toUpper()
value_t builtin_string_to_upper(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toUpper() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("toUpper() can only be called on strings");
    }

    ds_string result = ds_to_upper(receiver.as.string);
    return make_string_ds(result);
}

// String method: toLower()
value_t builtin_string_to_lower(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toLower() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("toLower() can only be called on strings");
    }

    ds_string result = ds_to_lower(receiver.as.string);
    return make_string_ds(result);
}

// String method: trim()
value_t builtin_string_trim(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("trim() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("trim() can only be called on strings");
    }

    ds_string result = ds_trim(receiver.as.string);
    return make_string_ds(result);
}

// String method: startsWith(prefix)
value_t builtin_string_starts_with(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("startsWith() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t prefix = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("startsWith() can only be called on strings");
    }
    if (prefix.type != VAL_STRING) {
        runtime_error("startsWith() argument must be a string");
    }

    int result = ds_starts_with(receiver.as.string, prefix.as.string);
    return make_boolean(result);
}

// String method: endsWith(suffix)
value_t builtin_string_ends_with(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("endsWith() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t suffix = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("endsWith() can only be called on strings");
    }
    if (suffix.type != VAL_STRING) {
        runtime_error("endsWith() argument must be a string");
    }

    int result = ds_ends_with(receiver.as.string, suffix.as.string);
    return make_boolean(result);
}

// String method: contains(needle)
value_t builtin_string_contains(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t needle = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("contains() can only be called on strings");
    }
    if (needle.type != VAL_STRING) {
        runtime_error("contains() argument must be a string");
    }

    int result = ds_contains(receiver.as.string, needle.as.string);
    return make_boolean(result);
}

// String method: replace(old, new)
value_t builtin_string_replace(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) {
        runtime_error("replace() takes exactly 2 arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t old_str = args[1];
    value_t new_str = args[2];

    if (receiver.type != VAL_STRING) {
        runtime_error("replace() can only be called on strings");
    }
    if (old_str.type != VAL_STRING || new_str.type != VAL_STRING) {
        runtime_error("replace() arguments must be strings");
    }

    ds_string result = ds_replace(receiver.as.string, old_str.as.string, new_str.as.string);
    return make_string_ds(result);
}

// String method: indexOf(needle)
value_t builtin_string_index_of(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("indexOf() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t needle = args[1];

    if (receiver.type != VAL_STRING) {
        runtime_error("indexOf() can only be called on strings");
    }
    if (needle.type != VAL_STRING) {
        runtime_error("indexOf() argument must be a string");
    }

    int result = ds_find(receiver.as.string, needle.as.string);
    return make_int32(result); // ds_find returns -1 if not found
}

// iterator(collection) - Create iterator for arrays and ranges
value_t builtin_iterator(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("iterator() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t collection = args[0];
    iterator_t* iter = NULL;

    switch (collection.type) {
    case VAL_ARRAY:
        iter = create_array_iterator(collection.as.array);
        break;
    case VAL_RANGE:
        if (collection.as.range) {
            iter = create_range_iterator(collection.as.range->start, collection.as.range->end,
                                         collection.as.range->exclusive);
        }
        break;
    default:
        runtime_error("iterator() can only be called on arrays and ranges, not %s", value_type_name(collection.type));
    }

    if (!iter) {
        runtime_error("Failed to create iterator");
    }

    return make_iterator(iter);
}

// hasNext(iterator) - Check if iterator has more elements
value_t builtin_has_next(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("hasNext() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error("hasNext() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }

    int has_next = iterator_has_next(iter_val.as.iterator);
    return make_boolean(has_next);
}

// next(iterator) - Get next element from iterator
value_t builtin_next(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("next() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error("next() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }

    if (!iterator_has_next(iter_val.as.iterator)) {
        runtime_error("Iterator has no more elements");
    }

    return iterator_next(iter_val.as.iterator);
}

// ========================
// BUFFER BUILTIN FUNCTIONS
// ========================

// buffer(data) - Create buffer from string or array
value_t builtin_buffer(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t data = args[0];
    db_buffer buf;

    if (data.type == VAL_STRING) {
        // Create buffer from string
        const char* str = data.as.string;
        size_t len = str ? strlen(str) : 0;
        buf = db_new_with_data(str, len);
    } else if (data.type == VAL_ARRAY) {
        // Create buffer from array of numbers
        da_array arr = data.as.array;
        size_t len = da_length(arr);

        // Convert array elements to bytes
        uint8_t* bytes = malloc(len);
        if (!bytes) {
            runtime_error("Failed to allocate memory for buffer");
        }

        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            if (!elem) {
                free(bytes);
                runtime_error("Invalid array element at index %zu", i);
            }
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0 || elem->as.int32 > 255) {
                    free(bytes);
                    runtime_error("Array element %d at index %zu is not a valid byte (0-255)", elem->as.int32, i);
                }
                bytes[i] = (uint8_t)elem->as.int32;
            } else {
                free(bytes);
                runtime_error("Array element at index %zu must be an integer, not %s", i, value_type_name(elem->type));
            }
        }

        buf = db_new_with_data(bytes, len);
        free(bytes);
    } else {
        runtime_error("buffer() requires a string or array argument, not %s", value_type_name(data.type));
    }

    return make_buffer(buf);
}

// buffer_from_hex(hex_string) - Create buffer from hex string
value_t builtin_buffer_from_hex(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_from_hex() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t hex_val = args[0];
    if (hex_val.type != VAL_STRING) {
        runtime_error("buffer_from_hex() requires a string argument, not %s", value_type_name(hex_val.type));
    }

    const char* hex_str = hex_val.as.string;
    if (!hex_str) {
        runtime_error("buffer_from_hex() requires a non-null string");
    }

    size_t len = strlen(hex_str);
    db_buffer buf = db_from_hex(hex_str, len);

    if (!buf) {
        runtime_error("Invalid hex string");
    }

    return make_buffer(buf);
}

// buffer_slice(buffer, offset, length) - Create buffer slice
value_t builtin_buffer_slice(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) {
        runtime_error("buffer_slice() takes exactly 3 arguments (%d given)", arg_count);
    }

    value_t buf_val = args[0];
    value_t offset_val = args[1];
    value_t length_val = args[2];

    if (buf_val.type != VAL_BUFFER) {
        runtime_error("buffer_slice() requires a buffer as first argument, not %s", value_type_name(buf_val.type));
    }
    if (offset_val.type != VAL_INT32) {
        runtime_error("buffer_slice() requires an integer offset, not %s", value_type_name(offset_val.type));
    }
    if (length_val.type != VAL_INT32) {
        runtime_error("buffer_slice() requires an integer length, not %s", value_type_name(length_val.type));
    }

    db_buffer buf = buf_val.as.buffer;
    int32_t offset = offset_val.as.int32;
    int32_t length = length_val.as.int32;

    if (offset < 0 || length < 0) {
        runtime_error("buffer_slice() offset and length must be non-negative");
    }

    db_buffer slice = db_slice(buf, (size_t)offset, (size_t)length);
    if (!slice) {
        runtime_error("Invalid buffer slice bounds");
    }

    return make_buffer(slice);
}

// buffer_concat(buf1, buf2) - Concatenate two buffers
value_t builtin_buffer_concat(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("buffer_concat() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t buf1_val = args[0];
    value_t buf2_val = args[1];

    if (buf1_val.type != VAL_BUFFER) {
        runtime_error("buffer_concat() requires buffer as first argument, not %s", value_type_name(buf1_val.type));
    }
    if (buf2_val.type != VAL_BUFFER) {
        runtime_error("buffer_concat() requires buffer as second argument, not %s", value_type_name(buf2_val.type));
    }

    db_buffer result = db_concat(buf1_val.as.buffer, buf2_val.as.buffer);
    return make_buffer(result);
}

// buffer_to_hex(buffer) - Convert buffer to hex string
value_t builtin_buffer_to_hex(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_to_hex() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buf_val = args[0];
    if (buf_val.type != VAL_BUFFER) {
        runtime_error("buffer_to_hex() requires a buffer argument, not %s", value_type_name(buf_val.type));
    }

    db_buffer hex_buf = db_to_hex(buf_val.as.buffer, false); // lowercase

    // Create null-terminated string from hex buffer
    size_t hex_len = db_size(hex_buf);
    char* null_term_hex = malloc(hex_len + 1);
    if (!null_term_hex) {
        db_release(&hex_buf);
        runtime_error("Failed to allocate memory for hex string");
    }

    memcpy(null_term_hex, hex_buf, hex_len);
    null_term_hex[hex_len] = '\0';

    ds_string hex_str = ds_new(null_term_hex);

    free(null_term_hex);
    db_release(&hex_buf);

    return make_string_ds(hex_str);
}

// =============================
// BUFFER BUILDER BUILTIN FUNCTIONS
// =============================

// buffer_builder(capacity) - Create buffer builder
value_t builtin_buffer_builder(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_builder() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t capacity_val = args[0];
    if (capacity_val.type != VAL_INT32) {
        runtime_error("buffer_builder() requires an integer capacity, not %s", value_type_name(capacity_val.type));
    }

    int32_t capacity = capacity_val.as.int32;
    if (capacity < 0) {
        runtime_error("buffer_builder() capacity must be non-negative");
    }

    // Create reference counted builder directly
    db_builder builder = db_builder_new((size_t)capacity);
    return make_buffer_builder(builder);
}

// builder_append_uint8(builder, value) - Append uint8 to builder
value_t builtin_builder_append_uint8(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint8() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint8() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint8() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 255) {
        runtime_error("builder_append_uint8() value must be 0-255, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint8(builder, (uint8_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint16_le(builder, value) - Append uint16 in little-endian
value_t builtin_builder_append_uint16_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint16_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint16_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint16_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0 || value > 65535) {
        runtime_error("builder_append_uint16_le() value must be 0-65535, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint16_le(builder, (uint16_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_uint32_le(builder, value) - Append uint32 in little-endian
value_t builtin_builder_append_uint32_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_uint32_le() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t value_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_uint32_le() requires a buffer builder, not %s",
                      value_type_name(builder_val.type));
    }
    if (value_val.type != VAL_INT32) {
        runtime_error("builder_append_uint32_le() requires an integer value, not %s", value_type_name(value_val.type));
    }

    int32_t value = value_val.as.int32;
    if (value < 0) {
        runtime_error("builder_append_uint32_le() value must be non-negative, got %d", value);
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_uint32_le(builder, (uint32_t)value) != 0) {
        runtime_error("Failed to append to buffer builder");
    }

    return make_null();
}

// builder_append_cstring(builder, string) - Append string to builder
value_t builtin_builder_append_cstring(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("builder_append_cstring() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    value_t string_val = args[1];

    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_append_cstring() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }
    if (string_val.type != VAL_STRING) {
        runtime_error("builder_append_cstring() requires a string value, not %s", value_type_name(string_val.type));
    }

    const char* str = string_val.as.string;
    if (!str) {
        runtime_error("builder_append_cstring() requires a non-null string");
    }

    db_builder builder = builder_val.as.builder;
    if (db_builder_append_cstring(builder, str) != 0) {
        runtime_error("Failed to append string to buffer builder");
    }

    return make_null();
}

// builder_finish(builder) - Finish building and get buffer
value_t builtin_builder_finish(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("builder_finish() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t builder_val = args[0];
    if (builder_val.type != VAL_BUFFER_BUILDER) {
        runtime_error("builder_finish() requires a buffer builder, not %s", value_type_name(builder_val.type));
    }

    db_builder builder = builder_val.as.builder;
    db_buffer result = db_builder_finish(&builder);

    // After finish, the builder is invalidated - reference counting handles cleanup

    return make_buffer(result);
}

// ============================
// BUFFER READER BUILTIN FUNCTIONS
// ============================

// buffer_reader(buffer) - Create buffer reader
value_t builtin_buffer_reader(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_reader() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    if (buffer_val.type != VAL_BUFFER) {
        runtime_error("buffer_reader() requires a buffer argument, not %s", value_type_name(buffer_val.type));
    }

    db_reader reader = db_reader_new(buffer_val.as.buffer);
    return make_buffer_reader(reader);
}

// reader_read_uint8(reader) - Read uint8 from reader
value_t builtin_reader_read_uint8(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint8() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint8() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 1)) {
        runtime_error("Cannot read uint8: not enough data remaining");
    }

    uint8_t value = db_read_uint8(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint16_le(reader) - Read uint16 in little-endian from reader
value_t builtin_reader_read_uint16_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint16_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint16_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 2)) {
        runtime_error("Cannot read uint16: not enough data remaining");
    }

    uint16_t value = db_read_uint16_le(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint32_le(reader) - Read uint32 in little-endian from reader
value_t builtin_reader_read_uint32_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint32_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint32_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 4)) {
        runtime_error("Cannot read uint32: not enough data remaining");
    }

    uint32_t value = db_read_uint32_le(reader);

    // Check if value fits in int32_t range
    if (value <= INT32_MAX) {
        return make_int32((int32_t)value);
    } else {
        // Convert to bigint for large values
        di_int big = di_from_uint32(value);
        return make_bigint(big);
    }
}

// reader_position(reader) - Get reader position
value_t builtin_reader_position(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_position() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_position() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t pos = db_reader_position(reader_val.as.reader);
    return make_int32((int32_t)pos);
}

// reader_remaining(reader) - Get remaining bytes in reader
value_t builtin_reader_remaining(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_remaining() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_remaining() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t remaining = db_reader_remaining(reader_val.as.reader);
    return make_int32((int32_t)remaining);
}

// ===================
// I/O BUILTIN FUNCTIONS
// ===================

// read_file(filename) - Read file into buffer
value_t builtin_read_file(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("read_file() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t filename_val = args[0];
    if (filename_val.type != VAL_STRING) {
        runtime_error("read_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error("read_file() requires a non-null filename");
    }

    db_buffer buf = db_read_file(filename);
    if (!buf) {
        runtime_error("Failed to read file: %s", filename);
    }

    return make_buffer(buf);
}

// write_file(buffer, filename) - Write buffer to file
value_t builtin_write_file(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("write_file() takes exactly 2 arguments (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    value_t filename_val = args[1];

    if (buffer_val.type != VAL_BUFFER) {
        runtime_error("write_file() requires a buffer as first argument, not %s", value_type_name(buffer_val.type));
    }
    if (filename_val.type != VAL_STRING) {
        runtime_error("write_file() requires a string filename, not %s", value_type_name(filename_val.type));
    }

    const char* filename = filename_val.as.string;
    if (!filename) {
        runtime_error("write_file() requires a non-null filename");
    }

    bool success = db_write_file(buffer_val.as.buffer, filename);
    return make_boolean(success);
}

// Array method: length()
// Returns the length of the array as an int32
value_t builtin_array_length(slate_vm* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the array)
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("length() can only be called on arrays");
    }
    
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: push(element)
// Adds element to end of array, returns new length
value_t builtin_array_push(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("push() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("push() can only be called on arrays");
    }
    
    // Add element to array
    da_push(receiver.as.array, &element);
    
    // Return new length
    size_t length = da_length(receiver.as.array);
    return make_int32((int32_t)length);
}

// Array method: pop()
// Removes and returns last element
value_t builtin_array_pop(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("pop() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("pop() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    if (length == 0) {
        return make_null(); // Return null for empty array
    }
    
    // Get last element
    value_t* last_elem = (value_t*)da_get(array, length - 1);
    value_t result = *last_elem; // Copy the value
    result = vm_retain(result); // Retain since we're returning it
    
    // Remove last element
    da_remove(array, length - 1, NULL);
    
    return result;
}

// Array method: isEmpty()
// Returns true if array has no elements
value_t builtin_array_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("isEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(is_empty);
}

// Array method: nonEmpty()
// Returns true if array has at least one element (Scala-inspired)
value_t builtin_array_non_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("nonEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("nonEmpty() can only be called on arrays");
    }
    
    bool is_empty = da_is_empty(receiver.as.array);
    return make_boolean(!is_empty);
}

// Array method: indexOf(element)
// Returns first index of element, or -1 if not found
value_t builtin_array_index_of(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("indexOf() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("indexOf() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    // Search for element
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(array, i);
        if (values_equal(*elem, element)) {
            return make_int32((int32_t)i);
        }
    }
    
    return make_int32(-1); // Not found
}

// Array method: contains(element)
// Returns true if array contains element
value_t builtin_array_contains(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) { // receiver + 1 arg
        runtime_error("contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t element = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("contains() can only be called on arrays");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    
    // Search for element
    for (size_t i = 0; i < length; i++) {
        value_t* elem = (value_t*)da_get(array, i);
        if (values_equal(*elem, element)) {
            return make_boolean(true);
        }
    }
    
    return make_boolean(false); // Not found
}

// Array method: copy()
// Returns a copy of the array
value_t builtin_array_copy(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("copy() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("copy() can only be called on arrays");
    }
    
    // Create a copy using da_copy
    da_array copy = da_copy(receiver.as.array);
    return make_array(copy);
}

// Array method: slice(start, end?)
// Returns a subset of the array from start to end (exclusive)
value_t builtin_array_slice(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count < 2 || arg_count > 3) {
        runtime_error("slice() takes 1 or 2 arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t start_val = args[1];
    
    if (receiver.type != VAL_ARRAY) {
        runtime_error("slice() can only be called on arrays");
    }
    
    if (start_val.type != VAL_INT32) {
        runtime_error("slice() start index must be an integer");
    }
    
    da_array array = receiver.as.array;
    size_t length = da_length(array);
    int start = start_val.as.int32;
    int end = (int)length; // Default to end of array
    
    // Handle optional end parameter
    if (arg_count == 3) {
        value_t end_val = args[2];
        if (end_val.type != VAL_INT32) {
            runtime_error("slice() end index must be an integer");
        }
        end = end_val.as.int32;
    }
    
    // Handle negative indices (Python-style)
    if (start < 0) start += (int)length;
    if (end < 0) end += (int)length;
    
    // Clamp to valid range
    if (start < 0) start = 0;
    if (end > (int)length) end = (int)length;
    if (start > end) start = end;
    
    // Create slice using da_slice
    da_array slice = da_slice(array, start, end);
    return make_array(slice);
}

// Array method: reverse()
// Reverses the array in-place and returns it
value_t builtin_array_reverse(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reverse() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ARRAY) {
        runtime_error("reverse() can only be called on arrays");
    }
    
    // Reverse the array in-place
    da_reverse(receiver.as.array);
    
    // Return the array (for chaining)
    return vm_retain(receiver);
}

// String method: isEmpty()
// Returns true if string has no characters
value_t builtin_string_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("isEmpty() can only be called on strings");
    }
    
    bool is_empty = (ds_length(receiver.as.string) == 0);
    return make_boolean(is_empty);
}

// String method: nonEmpty()
// Returns true if string has at least one character (Scala-inspired)
value_t builtin_string_non_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("nonEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_STRING) {
        runtime_error("nonEmpty() can only be called on strings");
    }
    
    bool is_empty = (ds_length(receiver.as.string) == 0);
    return make_boolean(!is_empty);
}

// Range method implementations

// range.start() - Get the starting value
value_t builtin_range_start(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("start() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("start() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    return vm_retain(range->start);
}

// range.end() - Get the ending value
value_t builtin_range_end(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("end() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("end() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    return vm_retain(range->end);
}

// range.isExclusive() - Check if range excludes end value
value_t builtin_range_is_exclusive(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isExclusive() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("isExclusive() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    return make_boolean(range->exclusive);
}

// range.isEmpty() - Check if range contains no elements
value_t builtin_range_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("isEmpty() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    // Range is empty if start > end, or if start == end and exclusive
    if (!is_number(range->start) || !is_number(range->end)) {
        // Non-numeric ranges are not empty by default
        return make_boolean(0);
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    
    if (start_val > end_val) {
        return make_boolean(1); // Empty - start > end
    }
    
    if (start_val == end_val && range->exclusive) {
        return make_boolean(1); // Empty - start == end with exclusive
    }
    
    return make_boolean(0); // Not empty
}

// range.length() - Number of elements in range
value_t builtin_range_length(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("length() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    // Only support numeric ranges for length calculation
    if (!is_number(range->start) || !is_number(range->end)) {
        runtime_error("length() only supported for numeric ranges");
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    
    if (start_val > end_val) {
        return make_int32(0); // Empty range
    }
    
    if (start_val == end_val) {
        return make_int32(range->exclusive ? 0 : 1);
    }
    
    // Calculate length based on integer step
    int start_int = (int)start_val;
    int end_int = (int)end_val;
    
    if (range->exclusive) {
        return make_int32(end_int - start_int);
    } else {
        return make_int32(end_int - start_int + 1);
    }
}

// range.contains(value) - Check if value is within range bounds
value_t builtin_range_contains(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t value = args[1];
    
    if (receiver.type != VAL_RANGE) {
        runtime_error("contains() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    // Only support numeric values for contains check
    if (!is_number(range->start) || !is_number(range->end) || !is_number(value)) {
        return make_boolean(0); // Non-numeric not contained
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    double check_val = value_to_double(value);
    
    if (range->exclusive) {
        return make_boolean(check_val >= start_val && check_val < end_val);
    } else {
        return make_boolean(check_val >= start_val && check_val <= end_val);
    }
}

// range.toArray() - Convert range to array
value_t builtin_range_to_array(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toArray() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("toArray() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    // Only support numeric ranges for conversion to array
    if (!is_number(range->start) || !is_number(range->end)) {
        runtime_error("toArray() only supported for numeric ranges");
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    
    da_array array = da_new(sizeof(value_t));
    
    if (start_val > end_val) {
        // Empty range - return empty array
        return make_array(array);
    }
    
    // Generate integer sequence
    int start_int = (int)start_val;
    int end_int = (int)end_val;
    
    if (range->exclusive) {
        for (int i = start_int; i < end_int; i++) {
            value_t val = make_int32(i);
            da_push(array, &val);
        }
    } else {
        for (int i = start_int; i <= end_int; i++) {
            value_t val = make_int32(i);
            da_push(array, &val);
        }
    }
    
    return make_array(array);
}

// range.reverse() - Create reversed range
value_t builtin_range_reverse(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reverse() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error("reverse() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error("Invalid range");
    }
    
    // Create reversed range: swap start and end, keep exclusivity
    return make_range(range->end, range->start, range->exclusive);
}

// range.equals(other) - Deep equality comparison
value_t builtin_range_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error("equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_RANGE) {
        runtime_error("equals() can only be called on ranges");
    }
    
    if (other.type != VAL_RANGE) {
        return make_boolean(0); // Different types not equal
    }
    
    range_t* range1 = receiver.as.range;
    range_t* range2 = other.as.range;
    
    if (!range1 || !range2) {
        return make_boolean(0); // Invalid ranges not equal
    }
    
    // Compare exclusivity flag
    if (range1->exclusive != range2->exclusive) {
        return make_boolean(0);
    }
    
    // Compare start and end values
    int start_equal = values_equal(range1->start, range2->start);
    int end_equal = values_equal(range1->end, range2->end);
    
    return make_boolean(start_equal && end_equal);
}

// Iterator method implementations

// iterator.isEmpty() - Check if iterator has no more elements
value_t builtin_iterator_is_empty(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ITERATOR) {
        runtime_error("isEmpty() can only be called on iterators");
    }
    
    iterator_t* iter = receiver.as.iterator;
    if (!iter) {
        runtime_error("Invalid iterator");
    }
    
    int has_next = iterator_has_next(iter);
    return make_boolean(!has_next); // isEmpty is !hasNext
}

// iterator.toArray() - Consume remaining elements into an array
value_t builtin_iterator_to_array(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("toArray() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ITERATOR) {
        runtime_error("toArray() can only be called on iterators");
    }
    
    iterator_t* iter = receiver.as.iterator;
    if (!iter) {
        runtime_error("Invalid iterator");
    }
    
    // Create new array to collect elements
    da_array array = da_new(sizeof(value_t));
    
    // Consume all remaining elements from iterator
    while (iterator_has_next(iter)) {
        value_t element = iterator_next(iter);
        da_push(array, &element);
    }
    
    return make_array(array);
}
