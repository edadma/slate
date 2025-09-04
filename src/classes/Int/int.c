#include "int.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "dynamic_int.h"
#include "number.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>

// Int method: hash() - Hash code for integers
value_t builtin_int_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_INT32) {
        runtime_error(vm, "hash() can only be called on int32");
    }
    
    // For 32-bit integers, the value itself is the hash
    return make_int32(receiver.as.int32);
}

// Int method: equals(other) - Equality comparison for integers  
value_t builtin_int_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    // Handle cross-type numeric equality (delegate to Number.equals)
    if (is_number(receiver) && is_number(other)) {
        value_t args[2] = { receiver, other };
        return builtin_number_equals(vm, 2, args);
    }
    
    // Non-numeric types are not equal to integers
    return make_boolean(0);
}

// Int method: abs() - Absolute value for integers
value_t builtin_int_abs(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "abs() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_INT32:
            if (receiver.as.int32 == INT32_MIN) {
                // INT32_MIN abs overflows to BigInt
                di_int result = di_from_int64((int64_t)INT32_MAX + 1);
                return make_bigint(result);
            }
            return make_int32(receiver.as.int32 < 0 ? -receiver.as.int32 : receiver.as.int32);
            
        case VAL_BIGINT:
            return make_bigint(di_abs(receiver.as.bigint));
            
        default:
            runtime_error(vm, "abs() can only be called on integers");
            return make_null();
    }
}

// Int method: sign() - Sign function for integers
value_t builtin_int_sign(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sign() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_INT32:
            if (receiver.as.int32 > 0) return make_int32(1);
            if (receiver.as.int32 < 0) return make_int32(-1);
            return make_int32(0);
            
        case VAL_BIGINT:
            if (di_is_zero(receiver.as.bigint)) return make_int32(0);
            if (di_is_negative(receiver.as.bigint)) return make_int32(-1);
            return make_int32(1);
            
        default:
            runtime_error(vm, "sign() can only be called on integers");
            return make_null();
    }
}

// Int method: isFinite() - Integers are always finite
value_t builtin_int_is_finite(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isFinite() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32 || receiver.type == VAL_BIGINT) {
        return make_boolean(1); // Integers are always finite
    } else {
        runtime_error(vm, "isFinite() can only be called on integers");
        return make_null();
    }
}

// Int method: isInteger() - Integers are always integers
value_t builtin_int_is_integer(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isInteger() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32 || receiver.type == VAL_BIGINT) {
        return make_boolean(1); // Integers are always integers
    } else {
        runtime_error(vm, "isInteger() can only be called on integers");
        return make_null();
    }
}

// Int method: sqrt() - Square root (returns float)
value_t builtin_int_sqrt(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sqrt() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        if (val < 0) {
            runtime_error(vm, "sqrt() cannot be applied to negative numbers");
        }
        return make_float64(sqrt(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        if (val < 0) {
            runtime_error(vm, "sqrt() cannot be applied to negative numbers");
        }
        return make_float64(sqrt(val));
    } else {
        runtime_error(vm, "sqrt() can only be called on integers");
        return make_null();
    }
}

// Int method: sin() - Sine function (returns float)
value_t builtin_int_sin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sin() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(sin(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(sin(val));
    } else {
        runtime_error(vm, "sin() can only be called on integers");
        return make_null();
    }
}

// Int method: cos() - Cosine function (returns float)
value_t builtin_int_cos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "cos() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(cos(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(cos(val));
    } else {
        runtime_error(vm, "cos() can only be called on integers");
        return make_null();
    }
}

// Int method: tan() - Tangent function (returns float)
value_t builtin_int_tan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "tan() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(tan(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(tan(val));
    } else {
        runtime_error(vm, "tan() can only be called on integers");
        return make_null();
    }
}

// Int method: exp() - Exponential function (returns float)
value_t builtin_int_exp(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "exp() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(exp(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(exp(val));
    } else {
        runtime_error(vm, "exp() can only be called on integers");
        return make_null();
    }
}

// Int method: ln() - Natural logarithm (returns float)
value_t builtin_int_ln(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "ln() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        if (val <= 0) {
            runtime_error(vm, "ln() argument must be positive");
        }
        return make_float64(log(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        if (val <= 0) {
            runtime_error(vm, "ln() argument must be positive");
        }
        return make_float64(log(val));
    } else {
        runtime_error(vm, "ln() can only be called on integers");
        return make_null();
    }
}

// Int method: asin() - Arc sine (returns float)
value_t builtin_int_asin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "asin() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        if (val < -1.0 || val > 1.0) {
            runtime_error(vm, "asin() argument must be between -1 and 1");
        }
        return make_float64(asin(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        if (val < -1.0 || val > 1.0) {
            runtime_error(vm, "asin() argument must be between -1 and 1");
        }
        return make_float64(asin(val));
    } else {
        runtime_error(vm, "asin() can only be called on integers");
        return make_null();
    }
}

// Int method: acos() - Arc cosine (returns float)
value_t builtin_int_acos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "acos() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        if (val < -1.0 || val > 1.0) {
            runtime_error(vm, "acos() argument must be between -1 and 1");
        }
        return make_float64(acos(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        if (val < -1.0 || val > 1.0) {
            runtime_error(vm, "acos() argument must be between -1 and 1");
        }
        return make_float64(acos(val));
    } else {
        runtime_error(vm, "acos() can only be called on integers");
        return make_null();
    }
}

// Int method: atan() - Arc tangent (returns float)
value_t builtin_int_atan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "atan() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(atan(val));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(atan(val));
    } else {
        runtime_error(vm, "atan() can only be called on integers");
        return make_null();
    }
}

// Int method: degrees() - Convert radians to degrees (returns float)
value_t builtin_int_degrees(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "degrees() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(val * (180.0 / M_PI));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(val * (180.0 / M_PI));
    } else {
        runtime_error(vm, "degrees() can only be called on integers");
        return make_null();
    }
}

// Int method: radians() - Convert degrees to radians (returns float)
value_t builtin_int_radians(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "radians() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_INT32) {
        double val = (double)receiver.as.int32;
        return make_float64(val * (M_PI / 180.0));
    } else if (receiver.type == VAL_BIGINT) {
        double val = di_to_double(receiver.as.bigint);
        return make_float64(val * (M_PI / 180.0));
    } else {
        runtime_error(vm, "radians() can only be called on integers");
        return make_null();
    }
}

// Helper function to check if a value is an integer (int32 or bigint)
int is_integer(value_t value) {
    return value.type == VAL_INT32 || value.type == VAL_BIGINT;
}

// Global Int class storage
value_t* global_int_class = NULL;

// Int factory function for converting strings to integers with optional base
value_t int_factory(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count == 0) {
        runtime_error(vm, "Int() requires at least 1 argument (the string to parse)");
        return make_null();
    }
    
    if (arg_count > 2) {
        runtime_error(vm, "Int() takes at most 2 arguments (string and optional base)");
        return make_null();
    }
    
    // First argument must be a string
    if (args[0].type != VAL_STRING) {
        runtime_error(vm, "Int() first argument must be a string");
        return make_null();
    }
    
    ds_string str = args[0].as.string;
    int base = 10; // Default base
    
    // If second argument provided, it must be the base
    if (arg_count == 2) {
        if (args[1].type != VAL_INT32) {
            runtime_error(vm, "Int() base argument must be an integer");
            return make_null();
        }
        base = args[1].as.int32;
        if (base < 2 || base > 36) {
            runtime_error(vm, "Int() base must be between 2 and 36, got %d", base);
            return make_null();
        }
    }
    
    // Parse the string
    char* endptr;
    long long result = strtoll(str, &endptr, base);
    
    // Check for parsing errors
    if (endptr == str) {
        runtime_error(vm, "Int() could not parse '%s' as integer in base %d", str, base);
        return make_null();
    }
    
    if (*endptr != '\0') {
        runtime_error(vm, "Int() found invalid characters in '%s' for base %d", str, base);
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
value_t safe_int_multiply(value_t a, value_t b) {
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
value_t builtin_int_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count < 1 || arg_count > 2) {
        runtime_error(vm, "toString() requires 0 or 1 arguments (optional base)");
        return make_null();
    }
    
    value_t receiver = args[0];
    int base = 10; // Default base
    
    if (arg_count == 2) {
        if (args[1].type != VAL_INT32) {
            runtime_error(vm, "toString() base argument must be an integer");
            return make_null();
        }
        base = args[1].as.int32;
        if (base < 2 || base > 36) {
            runtime_error(vm, "toString() base must be between 2 and 36, got %d", base);
            return make_null();
        }
    }
    
    // Convert based on type
    if (receiver.type == VAL_INT32) {
        char buffer[64];
        if (base == 10) {
            snprintf(buffer, sizeof(buffer), "%d", receiver.as.int32);
        } else if (base == 16) {
            snprintf(buffer, sizeof(buffer), "%x", receiver.as.int32);
        } else if (base == 2) {
            // Binary conversion
            if (receiver.as.int32 == 0) {
                return make_string("0");
            }
            char binary[33]; // 32 bits + null terminator
            binary[32] = '\0';
            int pos = 31;
            uint32_t num = (uint32_t)receiver.as.int32;
            while (num > 0 && pos >= 0) {
                binary[pos] = (num & 1) ? '1' : '0';
                num >>= 1;
                pos--;
            }
            return make_string(&binary[pos + 1]);
        } else {
            // General base conversion
            char result[64];
            int index = 0;
            int32_t num = receiver.as.int32;
            int negative = (num < 0);
            if (negative) num = -num;
            
            if (num == 0) {
                result[index++] = '0';
            } else {
                while (num > 0) {
                    int digit = num % base;
                    result[index++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                    num /= base;
                }
            }
            
            if (negative) result[index++] = '-';
            result[index] = '\0';
            
            // Reverse the string
            for (int i = 0; i < index / 2; i++) {
                char temp = result[i];
                result[i] = result[index - 1 - i];
                result[index - 1 - i] = temp;
            }
            
            return make_string(result);
        }
        return make_string(buffer);
    } else if (receiver.type == VAL_BIGINT) {
        char* str = di_to_string(receiver.as.bigint, base);
        value_t result = make_string(str);
        free(str);
        return result;
    } else {
        runtime_error(vm, "toString() can only be called on integers");
        return make_null();
    }
}

// Int method: setBit(position) - Set bit at position to 1
value_t builtin_int_set_bit(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "setBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_val = args[1];
    
    if (pos_val.type != VAL_INT32) {
        runtime_error(vm, "setBit() bit position must be an integer");
        return make_null();
    }
    
    int32_t position = pos_val.as.int32;
    if (position < 0) {
        runtime_error(vm, "setBit() bit position cannot be negative: %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error(vm, "setBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 | (1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, set the bit using bitwise operations
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted_bit = di_shift_left(bit_value, position);
        di_int final_result = di_or(result, shifted_bit);
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted_bit);
        return make_bigint(final_result);
    } else {
        runtime_error(vm, "setBit() can only be called on integers");
        return make_null();
    }
}

// Int method: clearBit(position) - Set bit at position to 0
value_t builtin_int_clear_bit(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "clearBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_val = args[1];
    
    if (pos_val.type != VAL_INT32) {
        runtime_error(vm, "clearBit() bit position must be an integer");
        return make_null();
    }
    
    int32_t position = pos_val.as.int32;
    if (position < 0) {
        runtime_error(vm, "clearBit() bit position cannot be negative: %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error(vm, "clearBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 & ~(1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, clear the bit
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted_bit = di_shift_left(bit_value, position);
        di_int inverted_bit = di_not(shifted_bit);
        di_int final_result = di_and(result, inverted_bit);
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted_bit);
        di_release(&inverted_bit);
        return make_bigint(final_result);
    } else {
        runtime_error(vm, "clearBit() can only be called on integers");
        return make_null();
    }
}

// Int method: toggleBit(position) - Toggle bit at position
value_t builtin_int_toggle_bit(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "toggleBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_val = args[1];
    
    if (pos_val.type != VAL_INT32) {
        runtime_error(vm, "toggleBit() bit position must be an integer");
        return make_null();
    }
    
    int32_t position = pos_val.as.int32;
    if (position < 0) {
        runtime_error(vm, "toggleBit() bit position cannot be negative: %d", position);
        return make_null();
    }
    
    if (receiver.type == VAL_INT32) {
        if (position >= 32) {
            runtime_error(vm, "toggleBit() position %d is out of range for 32-bit integer", position);
            return make_null();
        }
        int32_t result = receiver.as.int32 ^ (1 << position);
        return make_int32(result);
    } else if (receiver.type == VAL_BIGINT) {
        // For BigInt, toggle the bit
        di_int result = di_copy(receiver.as.bigint);
        di_int bit_value = di_from_int32(1);
        di_int shifted_bit = di_shift_left(bit_value, position);
        di_int final_result = di_xor(result, shifted_bit);
        di_release(&result);
        di_release(&bit_value);
        di_release(&shifted_bit);
        return make_bigint(final_result);
    } else {
        runtime_error(vm, "toggleBit() can only be called on integers");
        return make_null();
    }
}

// Int method: getBit(position) - Get bit value at position (0 or 1)
value_t builtin_int_get_bit(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "getBit() requires exactly 1 argument (bit position)");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t pos_val = args[1];
    
    if (pos_val.type != VAL_INT32) {
        runtime_error(vm, "getBit() bit position must be an integer");
        return make_null();
    }
    
    int32_t position = pos_val.as.int32;
    if (position < 0) {
        runtime_error(vm, "getBit() bit position cannot be negative: %d", position);
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
        
        int32_t result;
        if (di_to_int32(bit, &result)) {
            di_release(&shifted);
            di_release(&one);
            di_release(&bit);
            return make_int32(result);
        } else {
            di_release(&shifted);
            di_release(&one);
            di_release(&bit);
            return make_int32(0);
        }
    } else {
        runtime_error(vm, "getBit() can only be called on integers");
        return make_null();
    }
}

// Int method: countBits() - Count number of set bits (population count)
value_t builtin_int_count_bits(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "countBits() takes no arguments");
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
        runtime_error(vm, "countBits() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error(vm, "countBits() can only be called on integers");
        return make_null();
    }
}

// Int method: leadingZeros() - Count leading zero bits
value_t builtin_int_leading_zeros(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "leadingZeros() takes no arguments");
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
        // For BigInt, leading zeros - complex, for now return error
        runtime_error(vm, "leadingZeros() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error(vm, "leadingZeros() can only be called on integers");
        return make_null();
    }
}

// Int method: trailingZeros() - Count trailing zero bits
value_t builtin_int_trailing_zeros(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "trailingZeros() takes no arguments");
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
        // For BigInt, trailing zeros - complex, for now return error
        runtime_error(vm, "trailingZeros() not yet implemented for BigInt");
        return make_null();
    } else {
        runtime_error(vm, "trailingZeros() can only be called on integers");
        return make_null();
    }
}

// Int method: isEven() - Check if number is even
value_t builtin_int_is_even(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isEven() takes no arguments");
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
        runtime_error(vm, "isEven() can only be called on integers");
        return make_null();
    }
}

// Int method: isOdd() - Check if number is odd
value_t builtin_int_is_odd(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isOdd() takes no arguments");
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
        runtime_error(vm, "isOdd() can only be called on integers");
        return make_null();
    }
}

// Int method: isPrime() - Check if number is prime
value_t builtin_int_is_prime(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isPrime() takes no arguments");
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
            // Use the int32 logic for small BigInts
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
            runtime_error(vm, "isPrime() not yet implemented for large BigInt values");
            return make_null();
        }
    } else {
        runtime_error(vm, "isPrime() can only be called on integers");
        return make_null();
    }
}

// Int method: gcd(other) - Greatest common divisor
value_t builtin_int_gcd(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "gcd() requires exactly 1 argument");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (!is_integer(receiver) || !is_integer(other)) {
        runtime_error(vm, "gcd() requires integer arguments");
        return make_null();
    }
    
    // Handle int32 GCD
    if (receiver.type == VAL_INT32 && other.type == VAL_INT32) {
        int32_t a = abs(receiver.as.int32);
        int32_t b = abs(other.as.int32);
        
        while (b != 0) {
            int32_t temp = b;
            b = a % b;
            a = temp;
        }
        return make_int32(a);
    } else {
        // Convert to BigInt for mixed or BigInt cases
        di_int a_big = (receiver.type == VAL_INT32) ? di_from_int32(receiver.as.int32) : receiver.as.bigint;
        di_int b_big = (other.type == VAL_INT32) ? di_from_int32(other.as.int32) : other.as.bigint;
        
        di_int result = di_gcd(a_big, b_big);
        
        // Release temporary BigInts if we created them
        if (receiver.type == VAL_INT32) di_release(&a_big);
        if (other.type == VAL_INT32) di_release(&b_big);
        
        return make_bigint(result);
    }
}

// Int method: lcm(other) - Least common multiple
value_t builtin_int_lcm(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "lcm() requires exactly 1 argument");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (!is_integer(receiver) || !is_integer(other)) {
        runtime_error(vm, "lcm() requires integer arguments");
        return make_null();
    }
    
    // LCM(a,b) = |a*b| / GCD(a,b)
    // First calculate GCD
    value_t gcd_args[2] = { receiver, other };
    value_t gcd_result = builtin_int_gcd(vm, 2, gcd_args);
    
    // Calculate a * b
    value_t product = safe_int_multiply(receiver, other);
    
    // Calculate |a*b| / GCD(a,b)
    // For simplicity, we'll use the dynamic int library for division
    di_int product_big, gcd_big;
    
    if (product.type == VAL_INT32) {
        product_big = di_from_int32(abs(product.as.int32));
    } else {
        product_big = di_abs(product.as.bigint);
    }
    
    if (gcd_result.type == VAL_INT32) {
        gcd_big = di_from_int32(gcd_result.as.int32);
    } else {
        gcd_big = gcd_result.as.bigint;
    }
    
    di_int result = di_div(product_big, gcd_big);
    
    // Clean up temporary values
    if (product.type == VAL_INT32) di_release(&product_big);
    if (gcd_result.type == VAL_INT32) di_release(&gcd_big);
    vm_release(product);
    vm_release(gcd_result);
    
    return make_bigint(result);
}

// Int method: pow(exponent) - Integer exponentiation
value_t builtin_int_pow(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "pow() requires exactly 1 argument (exponent)");
        return make_null();
    }
    
    value_t base = args[0];
    value_t exp_val = args[1];
    
    if (!is_integer(base) || !is_integer(exp_val)) {
        runtime_error(vm, "pow() requires integer arguments");
        return make_null();
    }
    
    // Handle negative exponents
    if ((exp_val.type == VAL_INT32 && exp_val.as.int32 < 0) ||
        (exp_val.type == VAL_BIGINT && di_is_negative(exp_val.as.bigint))) {
        runtime_error(vm, "pow() does not support negative exponents");
        return make_null();
    }
    
    // Handle zero exponent
    if ((exp_val.type == VAL_INT32 && exp_val.as.int32 == 0) ||
        (exp_val.type == VAL_BIGINT && di_is_zero(exp_val.as.bigint))) {
        return make_int32(1);
    }
    
    // Handle simple int32 power for small values, otherwise error
    if (base.type == VAL_INT32 && exp_val.type == VAL_INT32) {
        int32_t base_int = base.as.int32;
        int32_t exp_int = exp_val.as.int32;
        
        if (exp_int < 0) {
            runtime_error(vm, "pow() exponent cannot be negative");
            return make_null();
        }
        
        // Handle simple cases
        if (exp_int == 0) return make_int32(1);
        if (base_int == 0) return make_int32(0);
        if (base_int == 1) return make_int32(1);
        if (base_int == -1) return make_int32(exp_int % 2 == 0 ? 1 : -1);
        
        // For other cases, compute using repeated multiplication with overflow check
        int64_t result = 1;
        int64_t base_64 = base_int;
        int32_t remaining_exp = exp_int;
        
        while (remaining_exp > 0) {
            if (remaining_exp & 1) {
                result *= base_64;
                // Check for overflow
                if (result > INT32_MAX || result < INT32_MIN) {
                    // Promote to BigInt
                    di_int big_result = di_from_int64(result);
                    return make_bigint(big_result);
                }
            }
            remaining_exp >>= 1;
            if (remaining_exp > 0) {
                base_64 *= base_64;
                if (base_64 > INT32_MAX || base_64 < INT32_MIN) {
                    // Promote to BigInt
                    di_int big_result = di_from_int64(result);
                    return make_bigint(big_result);
                }
            }
        }
        
        return make_int32((int32_t)result);
    } else {
        // For now, BigInt power is not fully implemented
        runtime_error(vm, "pow() not yet fully implemented for BigInt values");
        return make_null();
    }
}

// Int method: factorial() - Calculate factorial
value_t builtin_int_factorial(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "factorial() takes no arguments");
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (!is_integer(receiver)) {
        runtime_error(vm, "factorial() can only be called on integers");
        return make_null();
    }
    
    // Check for negative input
    if ((receiver.type == VAL_INT32 && receiver.as.int32 < 0) ||
        (receiver.type == VAL_BIGINT && di_is_negative(receiver.as.bigint))) {
        runtime_error(vm, "factorial() is not defined for negative numbers");
        return make_null();
    }
    
    // Handle small cases
    if (receiver.type == VAL_INT32 && receiver.as.int32 <= 1) {
        return make_int32(1);
    }
    
    // Only support VAL_INT32 for factorial
    if (receiver.type != VAL_INT32) {
        runtime_error(vm, "factorial() only supports 32-bit integers");
        return make_null();
    }
    
    // Calculate factorial using the int32 value
    di_int result = di_factorial((uint32_t)receiver.as.int32);
    
    return make_bigint(result);
}

// Initialize Int class with prototype and methods
void int_class_init(vm_t* vm) {
    // Create the Int class with its prototype
    do_object int_proto = do_create(NULL);
    
    // Add methods to Int prototype
    value_t int_hash_method = make_native(builtin_int_hash);
    do_set(int_proto, "hash", &int_hash_method, sizeof(value_t));
    
    value_t int_equals_method = make_native(builtin_int_equals);
    do_set(int_proto, "equals", &int_equals_method, sizeof(value_t));
    
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
    
    // Add Number interface methods for integers
    value_t int_abs_method = make_native(builtin_int_abs);
    do_set(int_proto, "abs", &int_abs_method, sizeof(value_t));
    
    value_t int_sign_method = make_native(builtin_int_sign);
    do_set(int_proto, "sign", &int_sign_method, sizeof(value_t));
    
    value_t int_is_finite_method = make_native(builtin_int_is_finite);
    do_set(int_proto, "isFinite", &int_is_finite_method, sizeof(value_t));
    
    value_t int_is_integer_method = make_native(builtin_int_is_integer);
    do_set(int_proto, "isInteger", &int_is_integer_method, sizeof(value_t));
    
    // Math methods for integers (return appropriate types)
    value_t int_sqrt_method = make_native(builtin_int_sqrt);
    do_set(int_proto, "sqrt", &int_sqrt_method, sizeof(value_t));
    
    value_t int_sin_method = make_native(builtin_int_sin);
    do_set(int_proto, "sin", &int_sin_method, sizeof(value_t));
    
    value_t int_cos_method = make_native(builtin_int_cos);
    do_set(int_proto, "cos", &int_cos_method, sizeof(value_t));
    
    value_t int_tan_method = make_native(builtin_int_tan);
    do_set(int_proto, "tan", &int_tan_method, sizeof(value_t));
    
    value_t int_exp_method = make_native(builtin_int_exp);
    do_set(int_proto, "exp", &int_exp_method, sizeof(value_t));
    
    value_t int_ln_method = make_native(builtin_int_ln);
    do_set(int_proto, "ln", &int_ln_method, sizeof(value_t));
    
    value_t int_asin_method = make_native(builtin_int_asin);
    do_set(int_proto, "asin", &int_asin_method, sizeof(value_t));
    
    value_t int_acos_method = make_native(builtin_int_acos);
    do_set(int_proto, "acos", &int_acos_method, sizeof(value_t));
    
    value_t int_atan_method = make_native(builtin_int_atan);
    do_set(int_proto, "atan", &int_atan_method, sizeof(value_t));
    
    value_t int_degrees_method = make_native(builtin_int_degrees);
    do_set(int_proto, "degrees", &int_degrees_method, sizeof(value_t));
    
    value_t int_radians_method = make_native(builtin_int_radians);
    do_set(int_proto, "radians", &int_radians_method, sizeof(value_t));
    
    // Create the Int class
    value_t int_class = make_class("Int", int_proto);
    
    // Set the factory function to allow Int(string, base?)
    int_class.as.class->factory = int_factory;
    
    // Get the Number class to inherit from
    value_t* number_class_ptr = (value_t*)do_get(vm->globals, "Number");
    if (number_class_ptr && number_class_ptr->type == VAL_CLASS) {
        // Set Number as parent class for inheritance
        int_class.class = number_class_ptr;
    }
    
    // Store in globals
    do_set(vm->globals, "Int", &int_class, sizeof(value_t));
    
    // Store a global reference for use in make_int32 and make_bigint
    static value_t int_class_storage;
    int_class_storage = vm_retain(int_class);
    global_int_class = &int_class_storage;
}