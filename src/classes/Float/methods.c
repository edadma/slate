#include "float.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "number.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// Float method: hash() - Hash code for floating-point numbers
value_t builtin_float_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_FLOAT32) {
        // Hash the bits of the float32
        union { float f; uint32_t u; } converter;
        converter.f = receiver.as.float32;
        // Special handling for NaN and -0.0
        if (isnan(converter.f)) {
            return make_int32(0x7fc00000); // Canonical NaN hash
        }
        if (converter.f == 0.0f) {
            return make_int32(0); // Both +0.0 and -0.0 hash to 0
        }
        return make_int32((int32_t)converter.u);
    } else if (receiver.type == VAL_FLOAT64) {
        // Hash the bits of the float64
        union { double d; uint64_t u; } converter;
        converter.d = receiver.as.float64;
        // Special handling for NaN and -0.0
        if (isnan(converter.d)) {
            return make_int32(0x7fc00000); // Canonical NaN hash
        }
        if (converter.d == 0.0) {
            return make_int32(0); // Both +0.0 and -0.0 hash to 0
        }
        // Mix high and low 32 bits
        uint32_t hash = (uint32_t)(converter.u ^ (converter.u >> 32));
        return make_int32((int32_t)hash);
    } else {
        runtime_error(vm, "hash() can only be called on floats");
    }
}

// Float method: equals(other) - Equality comparison for floats
value_t builtin_float_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    // Handle cross-type numeric equality (delegate to Number.equals)
    // This properly handles IEEE 754 semantics: NaN != NaN
    if (is_number(receiver) && is_number(other)) {
        value_t args[2] = { receiver, other };
        return builtin_number_equals(vm, 2, args);
    }
    
    // Non-numeric types are not equal to floats
    return make_boolean(0);
}

// Float method: toString([precision]) - Convert float to string representation
value_t builtin_float_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    if (receiver.type == VAL_FLOAT32) {
        float val = receiver.as.float32;
        if (isnan(val)) {
            return make_string("NaN");
        } else if (isinf(val)) {
            return make_string(val > 0 ? "Infinity" : "-Infinity");
        } else {
            char buffer[32];
            // Use %.7g for clean formatting without unnecessary zeros (float32 precision)
            snprintf(buffer, sizeof(buffer), "%.7g", val);
            return make_string(buffer);
        }
    } else if (receiver.type == VAL_FLOAT64) {
        double val = receiver.as.float64;
        if (isnan(val)) {
            return make_string("NaN");
        } else if (isinf(val)) {
            return make_string(val > 0 ? "Infinity" : "-Infinity");
        } else {
            char buffer[64];
            // Use %.15g for clean formatting without unnecessary zeros
            snprintf(buffer, sizeof(buffer), "%.15g", val);
            return make_string(buffer);
        }
    } else {
        runtime_error(vm, "toString() can only be called on floating point numbers");
        return make_null();
    }
}

// Float method: abs() - Absolute value for floats
value_t builtin_float_abs(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "abs() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(fabsf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(fabs(receiver.as.float64));
            
        default:
            runtime_error(vm, "abs() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: sign() - Sign function for floats
value_t builtin_float_sign(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sign() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            {
                float val = receiver.as.float32;
                if (isnan(val)) return make_float32(NAN); // NaN.sign() returns NaN
                if (val > 0.0f) return make_int32(1);
                if (val < 0.0f) return make_int32(-1);
                return make_int32(0);
            }
            
        case VAL_FLOAT64:
            {
                double val = receiver.as.float64;
                if (isnan(val)) return make_float64(NAN); // NaN.sign() returns NaN
                if (val > 0.0) return make_int32(1);
                if (val < 0.0) return make_int32(-1);
                return make_int32(0);
            }
            
        default:
            runtime_error(vm, "sign() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: isFinite() - Check if float is finite
value_t builtin_float_is_finite(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isFinite() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_boolean(isfinite(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_boolean(isfinite(receiver.as.float64));
            
        default:
            runtime_error(vm, "isFinite() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: isInteger() - Check if float represents integer value
value_t builtin_float_is_integer(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isInteger() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            {
                float val = receiver.as.float32;
                return make_boolean(isfinite(val) && floorf(val) == val);
            }
            
        case VAL_FLOAT64:
            {
                double val = receiver.as.float64;
                return make_boolean(isfinite(val) && floor(val) == val);
            }
            
        default:
            runtime_error(vm, "isInteger() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: sqrt() - Square root for floats
value_t builtin_float_sqrt(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sqrt() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(sqrtf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(sqrt(receiver.as.float64));
            
        default:
            runtime_error(vm, "sqrt() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: floor() - Floor function for floats
value_t builtin_float_floor(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "floor() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            double result = floor((double)receiver.as.float32);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        case VAL_FLOAT64: {
            double result = floor(receiver.as.float64);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        default:
            runtime_error(vm, "floor() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: ceil() - Ceiling function for floats
value_t builtin_float_ceil(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "ceil() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            double result = ceil((double)receiver.as.float32);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        case VAL_FLOAT64: {
            double result = ceil(receiver.as.float64);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        default:
            runtime_error(vm, "ceil() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: round() - Round function for floats
value_t builtin_float_round(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "round() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            double result = round((double)receiver.as.float32);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        case VAL_FLOAT64: {
            double result = round(receiver.as.float64);
            // Return int32 if it fits, otherwise float64
            if (result >= INT32_MIN && result <= INT32_MAX) {
                return make_int32((int32_t)result);
            } else {
                return make_float64(result);
            }
        }
            
        default:
            runtime_error(vm, "round() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: sin() - Sine function for floats
value_t builtin_float_sin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "sin() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(sinf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(sin(receiver.as.float64));
            
        default:
            runtime_error(vm, "sin() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: cos() - Cosine function for floats
value_t builtin_float_cos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "cos() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(cosf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(cos(receiver.as.float64));
            
        default:
            runtime_error(vm, "cos() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: tan() - Tangent function for floats
value_t builtin_float_tan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "tan() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(tanf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(tan(receiver.as.float64));
            
        default:
            runtime_error(vm, "tan() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: exp() - Exponential function for floats
value_t builtin_float_exp(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "exp() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(expf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(exp(receiver.as.float64));
            
        default:
            runtime_error(vm, "exp() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: ln() - Natural logarithm for floats
value_t builtin_float_ln(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "ln() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            float val = receiver.as.float32;
            if (val <= 0.0f) {
                runtime_error(vm, "ln() argument must be positive");
            }
            return make_float32(logf(val));
        }
            
        case VAL_FLOAT64: {
            double val = receiver.as.float64;
            if (val <= 0.0) {
                runtime_error(vm, "ln() argument must be positive");
            }
            return make_float64(log(val));
        }
            
        default:
            runtime_error(vm, "ln() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: asin() - Arc sine for floats
value_t builtin_float_asin(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "asin() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            float val = receiver.as.float32;
            if (val < -1.0f || val > 1.0f) {
                runtime_error(vm, "asin() argument must be between -1 and 1");
            }
            return make_float32(asinf(val));
        }
            
        case VAL_FLOAT64: {
            double val = receiver.as.float64;
            if (val < -1.0 || val > 1.0) {
                runtime_error(vm, "asin() argument must be between -1 and 1");
            }
            return make_float64(asin(val));
        }
            
        default:
            runtime_error(vm, "asin() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: acos() - Arc cosine for floats
value_t builtin_float_acos(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "acos() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32: {
            float val = receiver.as.float32;
            if (val < -1.0f || val > 1.0f) {
                runtime_error(vm, "acos() argument must be between -1 and 1");
            }
            return make_float32(acosf(val));
        }
            
        case VAL_FLOAT64: {
            double val = receiver.as.float64;
            if (val < -1.0 || val > 1.0) {
                runtime_error(vm, "acos() argument must be between -1 and 1");
            }
            return make_float64(acos(val));
        }
            
        default:
            runtime_error(vm, "acos() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: atan() - Arc tangent for floats
value_t builtin_float_atan(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "atan() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(atanf(receiver.as.float32));
            
        case VAL_FLOAT64:
            return make_float64(atan(receiver.as.float64));
            
        default:
            runtime_error(vm, "atan() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: degrees() - Convert radians to degrees
value_t builtin_float_degrees(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "degrees() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(receiver.as.float32 * (180.0f / M_PI));
            
        case VAL_FLOAT64:
            return make_float64(receiver.as.float64 * (180.0 / M_PI));
            
        default:
            runtime_error(vm, "degrees() can only be called on floating point numbers");
            return make_null();
    }
}

// Float method: radians() - Convert degrees to radians
value_t builtin_float_radians(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "radians() takes exactly 1 argument (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_FLOAT32:
            return make_float32(receiver.as.float32 * (M_PI / 180.0f));
            
        case VAL_FLOAT64:
            return make_float64(receiver.as.float64 * (M_PI / 180.0));
            
        default:
            runtime_error(vm, "radians() can only be called on floating point numbers");
            return make_null();
    }
}