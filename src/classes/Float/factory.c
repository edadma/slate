#include "float.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "dynamic_int.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Factory function for Float(value) constructor
value_t float_factory(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count == 0) {
        // Use build default for Float() with no arguments
#ifdef DEFAULT_FLOAT32
        return make_float32(0.0f);
#else
        return make_float64(0.0);
#endif
    }
    
    if (arg_count != 1) {
        runtime_error(vm, "Float() takes 0 or 1 argument (%d given)", arg_count);
    }
    
    value_t arg = args[0];
    
    switch (arg.type) {
        case VAL_INT32:
            return make_float64((double)arg.as.int32);
            
        case VAL_BIGINT:
            return make_float64(di_to_double(arg.as.bigint));
            
        case VAL_FLOAT32:
            return make_float64((double)arg.as.float32);
            
        case VAL_FLOAT64:
            return arg; // Already a float64
            
        case VAL_STRING:
            {
                const char* str = arg.as.string;
                char* endptr;
                double val = strtod(str, &endptr);
                
                // Check for type suffix like the lexer does
                int force_float32 = 0;
                int force_float64 = 0;
                
                if (*endptr == 'f' || *endptr == 'F') {
                    force_float32 = 1;
                    endptr++;
                } else if (*endptr == 'd' || *endptr == 'D') {
                    force_float64 = 1;
                    endptr++;
                }
                
                if (*endptr != '\0') {
                    runtime_error(vm, "Float() argument must be a valid number string");
                }
                
                // Respect build configuration and suffixes
                if (force_float32) {
                    return make_float32((float)val);
                } else if (force_float64) {
                    return make_float64(val);
                } else {
                    // Use build default like the lexer does
#ifdef DEFAULT_FLOAT32
                    return make_float32((float)val);
#else
                    return make_float64(val);
#endif
                }
            }
            
        default:
            runtime_error(vm, "Float() argument must be a number or string");
    }
}