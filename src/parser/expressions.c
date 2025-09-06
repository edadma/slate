#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// Forward declarations for static functions
static ast_node* parse_bitwise_or(parser_t* parser);
static ast_node* parse_bitwise_xor(parser_t* parser);
static ast_node* parse_bitwise_and(parser_t* parser);
static ast_node* parse_shift(parser_t* parser);
static ast_node* parse_template_literal(parser_t* parser);

// Parse null coalescing (??)
static ast_node* parse_null_coalesce(parser_t* parser) {
    ast_node* expr = parse_or(parser);
    
    while (parser_match(parser, TOKEN_NULL_COALESCE)) {
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_or(parser);
        expr = (ast_node*)ast_create_binary_op(BIN_NULL_COALESCE, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse ternary conditional (? :)
static ast_node* parse_ternary(parser_t* parser) {
    ast_node* expr = parse_null_coalesce(parser);
    
    if (parser_match(parser, TOKEN_QUESTION)) {
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* true_expr = parse_assignment(parser);
        parser_consume(parser, TOKEN_COLON, "Expected ':' after true expression in ternary");
        ast_node* false_expr = parse_ternary(parser);
        expr = (ast_node*)ast_create_ternary(expr, true_expr, false_expr, op_line, op_column);
    }
    
    return expr;
}

// Parse expression
ast_node* parse_expression(parser_t* parser) {
    return parse_assignment(parser);
}

// Parse assignment
ast_node* parse_assignment(parser_t* parser) {
    ast_node* expr = parse_ternary(parser);
    
    if (parser_match(parser, TOKEN_ASSIGN)) {
        ast_node* value = parse_assignment(parser);
        
        return (ast_node*)ast_create_assignment(expr, value,
                                               parser->previous.line, parser->previous.column);
    }
    
    // Handle compound assignments
    if (parser_check(parser, TOKEN_PLUS_ASSIGN) || parser_check(parser, TOKEN_MINUS_ASSIGN) ||
        parser_check(parser, TOKEN_MULT_ASSIGN) || parser_check(parser, TOKEN_DIV_ASSIGN) ||
        parser_check(parser, TOKEN_MOD_ASSIGN) || parser_check(parser, TOKEN_POWER_ASSIGN) ||
        parser_check(parser, TOKEN_FLOOR_DIV_ASSIGN) ||
        parser_check(parser, TOKEN_BITWISE_AND_ASSIGN) || parser_check(parser, TOKEN_BITWISE_OR_ASSIGN) ||
        parser_check(parser, TOKEN_BITWISE_XOR_ASSIGN) || parser_check(parser, TOKEN_LEFT_SHIFT_ASSIGN) ||
        parser_check(parser, TOKEN_RIGHT_SHIFT_ASSIGN) || parser_check(parser, TOKEN_LOGICAL_RIGHT_SHIFT_ASSIGN) ||
        parser_check(parser, TOKEN_LOGICAL_AND_ASSIGN) || parser_check(parser, TOKEN_LOGICAL_OR_ASSIGN) ||
        parser_check(parser, TOKEN_NULL_COALESCE_ASSIGN)) {
        
        token_t op_token = parser->current;
        parser_advance(parser);  // consume the compound assignment operator
        ast_node* value = parse_assignment(parser);
        
        // Map compound assignment tokens to binary operators
        binary_operator binary_op;
        switch (op_token.type) {
            case TOKEN_PLUS_ASSIGN:  binary_op = BIN_ADD; break;
            case TOKEN_MINUS_ASSIGN: binary_op = BIN_SUBTRACT; break;
            case TOKEN_MULT_ASSIGN:  binary_op = BIN_MULTIPLY; break;
            case TOKEN_DIV_ASSIGN:   binary_op = BIN_DIVIDE; break;
            case TOKEN_MOD_ASSIGN:   binary_op = BIN_MOD; break;
            case TOKEN_POWER_ASSIGN: binary_op = BIN_POWER; break;
            case TOKEN_FLOOR_DIV_ASSIGN: binary_op = BIN_FLOOR_DIV; break;
            case TOKEN_BITWISE_AND_ASSIGN: binary_op = BIN_BITWISE_AND; break;
            case TOKEN_BITWISE_OR_ASSIGN:  binary_op = BIN_BITWISE_OR; break;
            case TOKEN_BITWISE_XOR_ASSIGN: binary_op = BIN_BITWISE_XOR; break;
            case TOKEN_LEFT_SHIFT_ASSIGN: binary_op = BIN_LEFT_SHIFT; break;
            case TOKEN_RIGHT_SHIFT_ASSIGN: binary_op = BIN_RIGHT_SHIFT; break;
            case TOKEN_LOGICAL_RIGHT_SHIFT_ASSIGN: binary_op = BIN_LOGICAL_RIGHT_SHIFT; break;
            case TOKEN_LOGICAL_AND_ASSIGN: binary_op = BIN_LOGICAL_AND; break;
            case TOKEN_LOGICAL_OR_ASSIGN:  binary_op = BIN_LOGICAL_OR; break;
            case TOKEN_NULL_COALESCE_ASSIGN: binary_op = BIN_NULL_COALESCE; break;
            default: 
                parser_error_at(parser, &op_token, "Unknown compound assignment operator");
                return expr;
        }
        
        return (ast_node*)ast_create_compound_assignment(expr, value, binary_op,
                                                        op_token.line, op_token.column);
    }
    
    return expr;
}

// Parse logical OR
ast_node* parse_or(parser_t* parser) {
    ast_node* expr = parse_and(parser);
    
    while (parser_match(parser, TOKEN_LOGICAL_OR) || parser_match(parser, TOKEN_OR)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_and(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse logical AND
ast_node* parse_and(parser_t* parser) {
    ast_node* expr = parse_bitwise_or(parser);
    
    while (parser_match(parser, TOKEN_LOGICAL_AND) || parser_match(parser, TOKEN_AND)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_bitwise_or(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse bitwise OR
static ast_node* parse_bitwise_or(parser_t* parser) {
    ast_node* expr = parse_bitwise_xor(parser);
    
    while (parser_match(parser, TOKEN_BITWISE_OR)) {
        binary_operator op = BIN_BITWISE_OR;
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_bitwise_xor(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse bitwise XOR
static ast_node* parse_bitwise_xor(parser_t* parser) {
    ast_node* expr = parse_bitwise_and(parser);
    
    while (parser_match(parser, TOKEN_BITWISE_XOR)) {
        binary_operator op = BIN_BITWISE_XOR;
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_bitwise_and(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse bitwise AND
static ast_node* parse_bitwise_and(parser_t* parser) {
    ast_node* expr = parse_equality(parser);
    
    while (parser_match(parser, TOKEN_BITWISE_AND)) {
        binary_operator op = BIN_BITWISE_AND;
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_equality(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse equality
ast_node* parse_equality(parser_t* parser) {
    ast_node* expr = parse_comparison(parser);
    
    while (parser_match(parser, TOKEN_EQUAL) || parser_match(parser, TOKEN_NOT_EQUAL)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_comparison(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse comparison
ast_node* parse_comparison(parser_t* parser) {
    ast_node* expr = parse_range(parser);
    
    while (parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL) ||
           parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_IN) || parser_match(parser, TOKEN_INSTANCEOF)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_range(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse range expressions
ast_node* parse_range(parser_t* parser) {
    ast_node* expr = parse_shift(parser);
    
    if (parser_match(parser, TOKEN_RANGE) || parser_match(parser, TOKEN_RANGE_EXCLUSIVE)) {
        int exclusive = (parser->previous.type == TOKEN_RANGE_EXCLUSIVE);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* end = parse_shift(parser);
        
        // Check for optional step clause: "step <expression>"
        ast_node* step = NULL;
        if (parser_match(parser, TOKEN_STEP)) {
            step = parse_shift(parser);
        }
        
        expr = (ast_node*)ast_create_range(expr, end, exclusive, step, op_line, op_column);
    }
    
    return expr;
}

// Parse shift operations
static ast_node* parse_shift(parser_t* parser) {
    ast_node* expr = parse_term(parser);
    
    while (parser_match(parser, TOKEN_LEFT_SHIFT) || parser_match(parser, TOKEN_RIGHT_SHIFT) || 
           parser_match(parser, TOKEN_LOGICAL_RIGHT_SHIFT)) {
        binary_operator op = (parser->previous.type == TOKEN_LEFT_SHIFT) ? BIN_LEFT_SHIFT :
                            (parser->previous.type == TOKEN_RIGHT_SHIFT) ? BIN_RIGHT_SHIFT : BIN_LOGICAL_RIGHT_SHIFT;
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_term(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse term (+ -)
ast_node* parse_term(parser_t* parser) {
    ast_node* expr = parse_factor(parser);
    
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_factor(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Forward declaration for parse_power
static ast_node* parse_power(parser_t* parser);

// Parse factor (* /)
ast_node* parse_factor(parser_t* parser) {
    ast_node* expr = parse_power(parser);
    
    while (parser_match(parser, TOKEN_MULTIPLY) || parser_match(parser, TOKEN_DIVIDE) || 
           parser_match(parser, TOKEN_MOD) || parser_match(parser, TOKEN_FLOOR_DIV)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_power(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

static ast_node* parse_power(parser_t* parser) {
    ast_node* expr = parse_unary(parser);
    
    // Power is right-associative
    if (parser_match(parser, TOKEN_POWER)) {
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_power(parser); // Recursive for right-associativity
        expr = (ast_node*)ast_create_binary_op(BIN_POWER, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse unary (! - ~ ++ --)
ast_node* parse_unary(parser_t* parser) {
    // Prefix operators
    if (parser_match(parser, TOKEN_LOGICAL_NOT) || parser_match(parser, TOKEN_NOT) || 
        parser_match(parser, TOKEN_MINUS) || parser_match(parser, TOKEN_BITWISE_NOT) ||
        parser_match(parser, TOKEN_INCREMENT) || parser_match(parser, TOKEN_DECREMENT)) {
        unary_operator op = token_to_unary_op(parser->previous.type);
        ast_node* right = parse_unary(parser);
        return (ast_node*)ast_create_unary_op(op, right,
                                             parser->previous.line, parser->previous.column);
    }
    
    return parse_postfix(parser);
}

// Parse postfix operations (++ --)
ast_node* parse_postfix(parser_t* parser) {
    ast_node* expr = parse_call(parser);
    
    if (parser_match(parser, TOKEN_INCREMENT)) {
        return (ast_node*)ast_create_unary_op(UN_POST_INCREMENT, expr,
                                             parser->previous.line, parser->previous.column);
    } else if (parser_match(parser, TOKEN_DECREMENT)) {
        return (ast_node*)ast_create_unary_op(UN_POST_DECREMENT, expr,
                                             parser->previous.line, parser->previous.column);
    }
    
    return expr;
}

// Parse call
ast_node* parse_call(parser_t* parser) {
    ast_node* expr = parse_primary(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_LEFT_PAREN)) {
            expr = finish_call(parser, expr);
        } else if (parser_match(parser, TOKEN_DOT)) {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected property name after '.'.");
            char* name = token_to_string(&parser->previous);
            expr = (ast_node*)ast_create_member(expr, name, 0, // not optional
                                               parser->previous.line, parser->previous.column);
            free(name);
        } else if (parser_match(parser, TOKEN_OPTIONAL_CHAIN)) {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected property name after '?.'.");
            char* name = token_to_string(&parser->previous);
            expr = (ast_node*)ast_create_member(expr, name, 1, // is optional
                                               parser->previous.line, parser->previous.column);
            free(name);
        } else {
            break;
        }
    }
    
    return expr;
}

// Finish function call
ast_node* finish_call(parser_t* parser, ast_node* callee) {
    ast_node** arguments = NULL;
    size_t arg_count = 0;
    size_t arg_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            ast_node* arg = parse_expression(parser);
            
            if (arg_count >= arg_capacity) {
                size_t new_capacity = arg_capacity == 0 ? 8 : arg_capacity * 2;
                arguments = realloc(arguments, sizeof(ast_node*) * new_capacity);
                arg_capacity = new_capacity;
            }
            
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
    
    return (ast_node*)ast_create_call(callee, arguments, arg_count,
                                     parser->previous.line, parser->previous.column);
}

// Parse primary expressions
ast_node* parse_primary(parser_t* parser) {
    if (parser_match(parser, TOKEN_TRUE)) {
        return (ast_node*)ast_create_boolean(1, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_FALSE)) {
        return (ast_node*)ast_create_boolean(0, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_NULL)) {
        return (ast_node*)ast_create_null(parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_UNDEFINED)) {
        return (ast_node*)ast_create_undefined(parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_NAN)) {
        // Create NaN as float64 by default, respecting build configuration  
#ifdef DEFAULT_FLOAT32
        return (ast_node*)ast_create_float32(NAN, parser->previous.line, parser->previous.column);
#else
        return (ast_node*)ast_create_float64(NAN, parser->previous.line, parser->previous.column);
#endif
    }
    
    if (parser_match(parser, TOKEN_INFINITY)) {
        // Create Infinity as float64 by default, respecting build configuration
#ifdef DEFAULT_FLOAT32
        return (ast_node*)ast_create_float32(INFINITY, parser->previous.line, parser->previous.column);
#else
        return (ast_node*)ast_create_float64(INFINITY, parser->previous.line, parser->previous.column);
#endif
    }
    
    if (parser_match(parser, TOKEN_INTEGER)) {
        // Extract token string
        char buffer[256];
        size_t len = parser->previous.length < 255 ? parser->previous.length : 255;
        memcpy(buffer, parser->previous.start, len);
        buffer[len] = '\0';
        
        // Check if this is a hexadecimal literal
        if (buffer[0] == '0' && (buffer[1] == 'x' || buffer[1] == 'X')) {
            // Hexadecimal literal - parse with dynamic int
            di_int big_value = di_from_string(buffer + 2, 16);
            if (big_value == NULL) {
                parser_error(parser, "Invalid hexadecimal literal");
                return NULL;
            }
            
            // Check if it fits in 32-bit range
            int32_t int32_val;
            if (di_to_int32(big_value, &int32_val)) {
                // Fits in int32 - create integer AST node and release the BigInt
                di_release(&big_value);
                return (ast_node*)ast_create_integer(int32_val, parser->previous.line, parser->previous.column);
            } else {
                // Too large for int32 - create BigInt AST node (transfers ownership)
                return (ast_node*)ast_create_bigint(big_value, parser->previous.line, parser->previous.column);
            }
        } else {
            // Regular decimal integer - use original logic
            char* endptr;
            errno = 0;
            long long value = strtoll(buffer, &endptr, 10);
            
            if (errno == ERANGE || value < INT32_MIN || value > INT32_MAX) {
                // Too large for int32, create as BigInt
                di_int big_value = di_from_string(buffer, 10);
                if (big_value == NULL) {
                    // Fallback to double for very large numbers
                    double dval = strtod(buffer, NULL);
                    return (ast_node*)ast_create_number(dval, parser->previous.line, parser->previous.column);
                }
                return (ast_node*)ast_create_bigint(big_value, parser->previous.line, parser->previous.column);
            } else {
                // Fits in int32 - create integer AST node
                return (ast_node*)ast_create_integer((int32_t)value, parser->previous.line, parser->previous.column);
            }
        }
    }
    
    if (parser_match(parser, TOKEN_FLOAT32)) {
        float value = token_to_float32(&parser->previous);
        return (ast_node*)ast_create_float32(value, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_FLOAT64)) {
        double value = token_to_number(&parser->previous);
        return (ast_node*)ast_create_float64(value, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_NUMBER)) {
        double value = token_to_number(&parser->previous);
        return (ast_node*)ast_create_number(value, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_STRING)) {
        // Remove quotes from string
        char* str = token_to_string(&parser->previous);
        if (str && strlen(str) >= 2) {
            // Remove surrounding quotes
            memmove(str, str + 1, strlen(str) - 2);
            str[strlen(str) - 2] = '\0';
            
            // Process escape sequences
            char* dst = str;
            char* src = str;
            while (*src) {
                if (*src == '\\' && *(src + 1)) {
                    src++; // Skip backslash
                    switch (*src) {
                        case 'n': *dst++ = '\n'; break;
                        case 't': *dst++ = '\t'; break;
                        case '\\': *dst++ = '\\'; break;
                        case '"': *dst++ = '"'; break;
                        default: *dst++ = *src; break; // Unknown escape, keep the character
                    }
                    src++;
                } else {
                    *dst++ = *src++;
                }
            }
            *dst = '\0';
        }
        ast_node* result = (ast_node*)ast_create_string(str, parser->previous.line, parser->previous.column);
        free(str);
        return result;
    }
    
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        // Check for single-parameter lambda: IDENTIFIER -> expression
        if (parser_check(parser, TOKEN_ARROW)) {
            // This is a single-parameter lambda: x -> expr
            char* param_name = token_to_string(&parser->previous);
            char** parameters = malloc(sizeof(char*));
            parameters[0] = param_name;
            
            return parse_arrow_function(parser, parameters, 1);
        } else {
            // Regular identifier
            char* name = token_to_string(&parser->previous);
            ast_node* result = (ast_node*)ast_create_identifier(name, parser->previous.line, parser->previous.column);
            free(name);
            return result;
        }
    }
    
    if (parser_match(parser, TOKEN_TEMPLATE_START)) {
        return parse_template_literal(parser);
    }
    
    if (parser_match(parser, TOKEN_LEFT_PAREN)) {
        // Check if this is an arrow function parameter list
        return parse_parenthesized_or_arrow(parser);
    }
    
    if (parser_match(parser, TOKEN_LEFT_BRACKET)) {
        return parse_array(parser);
    }
    
    if (parser_match(parser, TOKEN_LEFT_BRACE)) {
        // Braces are now only for object literals
        return parse_object(parser);
    }
    
    if (parser_match(parser, TOKEN_IF)) {
        return parse_if_expression(parser);
    }
    
    if (parser_match(parser, TOKEN_MATCH)) {
        return parse_match_expression(parser);
    }
    
    if (parser_match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_FOR)) {
        return parse_for_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_DO)) {
        return parse_do_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_LOOP)) {
        return parse_loop_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_BREAK)) {
        return parse_break_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_CONTINUE)) {
        return parse_continue_statement(parser);
    }
    
    // Check for indented block as expression
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        return parse_indented_block(parser);
    }
    
    parser_error_at_current(parser, "Expected expression.");
    // Advance to avoid infinite loop on error
    parser_advance(parser);
    return NULL;
}

// Parse arrow function
ast_node* parse_arrow_function(parser_t* parser, char** parameters, size_t param_count) {
    // Consume the '->' token
    parser_consume(parser, TOKEN_ARROW, "Expected '->' in arrow function");
    
    // Parse the function body (expression or indented block)
    ast_node* body = parse_expression(parser);
    
    return (ast_node*)ast_create_function(parameters, param_count, body, 1, 
                                         parser->previous.line, parser->previous.column);
}

// Parse either a parenthesized expression or arrow function parameter list
ast_node* parse_parenthesized_or_arrow(parser_t* parser) {
    // We've already consumed the opening '('
    
    // Handle empty parameter list: () -> expr
    if (parser_check(parser, TOKEN_RIGHT_PAREN)) {
        parser_advance(parser); // consume ')'
        
        if (parser_check(parser, TOKEN_ARROW)) {
            // This is an empty parameter arrow function: () -> expr
            return parse_arrow_function(parser, NULL, 0);
        } else {
            parser_error(parser, "Empty parentheses without arrow function");
            return NULL;
        }
    }
    
    // Look ahead to determine if this is parameters or a grouped expression
    // We'll parse identifiers and see if we hit a ',' or ')' followed by '->'
    
    char** parameters = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;
    
    // Use two-token lookahead to determine parsing strategy
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_advance(parser); // consume first identifier
        
        if (parser_check(parser, TOKEN_ARROW)) {
            // Pattern: (x -> ...) - single-parameter lambda in parentheses
            char* param_name = token_to_string(&parser->previous);
            char** parameters = malloc(sizeof(char*));
            parameters[0] = param_name;
            
            ast_node* lambda = parse_arrow_function(parser, parameters, 1);
            parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after lambda expression");
            return lambda;
            
        } else if (parser_check(parser, TOKEN_COMMA) || parser_check(parser, TOKEN_RIGHT_PAREN)) {
            // Pattern: (x, ...) or (x) - multi-parameter lambda, start building list
            char* param = token_to_string(&parser->previous);
            
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                parameters = realloc(parameters, param_capacity * sizeof(char*));
            }
            parameters[param_count++] = param;
            
            // Continue with comma-separated parameter list parsing
            while (parser_match(parser, TOKEN_COMMA)) {
                if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                    // Clean up and error
                    for (size_t i = 0; i < param_count; i++) {
                        free(parameters[i]);
                    }
                    free(parameters);
                    parser_error(parser, "Expected parameter name");
                    return NULL;
                }
                
                char* param = token_to_string(&parser->previous);
                
                if (param_count >= param_capacity) {
                    param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                    parameters = realloc(parameters, param_capacity * sizeof(char*));
                }
                parameters[param_count++] = param;
            }
            
            // Should now be at ')'
            parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
            
            if (parser_check(parser, TOKEN_ARROW)) {
                // This is definitely an arrow function
                return parse_arrow_function(parser, parameters, param_count);
            } else {
                // This looked like parameters but no arrow - error
                for (size_t i = 0; i < param_count; i++) {
                    free(parameters[i]);
                }
                free(parameters);
                parser_error(parser, "Expected '->' after parameter list");
                return NULL;
            }
            
        } else {
            // This is not a lambda parameter, must be a grouped expression
            // Push back the identifier and parse as grouped expression
            parser_pushback(parser);
            ast_node* expr = parse_expression(parser);
            parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
            return expr;
        }
    } else {
        // Not an identifier, so this must be a grouped expression
        ast_node* expr = parse_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
}

// Parse array literal
ast_node* parse_array(parser_t* parser) {
    ast_node** elements = NULL;
    size_t element_count = 0;
    size_t element_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            ast_node* element = parse_expression(parser);
            
            if (element_count >= element_capacity) {
                size_t new_capacity = element_capacity == 0 ? 8 : element_capacity * 2;
                elements = realloc(elements, sizeof(ast_node*) * new_capacity);
                element_capacity = new_capacity;
            }
            
            elements[element_count++] = element;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after array elements.");
    
    return (ast_node*)ast_create_array(elements, element_count,
                                      parser->previous.line, parser->previous.column);
}

// Parse object literal  
ast_node* parse_object(parser_t* parser) {
    object_property* properties = NULL;
    size_t property_count = 0;
    size_t property_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            // Skip newlines inside object
            while (parser_match(parser, TOKEN_NEWLINE));
            
            if (parser_check(parser, TOKEN_RIGHT_BRACE)) break;
            
            // Parse key (identifier or string)
            char* key = NULL;
            if (parser_match(parser, TOKEN_IDENTIFIER)) {
                key = token_to_string(&parser->previous);
            } else if (parser_match(parser, TOKEN_STRING)) {
                key = token_to_string(&parser->previous);
                // Remove quotes
                if (key && strlen(key) >= 2) {
                    memmove(key, key + 1, strlen(key) - 2);
                    key[strlen(key) - 2] = '\0';
                }
            } else {
                parser_error_at_current(parser, "Expected property name.");
                break;
            }
            
            parser_consume(parser, TOKEN_COLON, "Expected ':' after property name.");
            ast_node* value = parse_expression(parser);
            
            if (property_count >= property_capacity) {
                size_t new_capacity = property_capacity == 0 ? 8 : property_capacity * 2;
                properties = realloc(properties, sizeof(object_property) * new_capacity);
                property_capacity = new_capacity;
            }
            
            properties[property_count].key = key;
            properties[property_count].value = value;
            property_count++;
            
            // Skip newlines after property
            while (parser_match(parser, TOKEN_NEWLINE));
            
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after object properties.");
    
    return (ast_node*)ast_create_object_literal(properties, property_count,
                                               parser->previous.line, parser->previous.column);
}

// Parse template literal (backtick strings with interpolation)
static ast_node* parse_template_literal(parser_t* parser) {
    // We've already consumed TOKEN_TEMPLATE_START
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    template_part* parts = NULL;
    size_t part_count = 0;
    size_t part_capacity = 0;
    
    while (!parser_check(parser, TOKEN_TEMPLATE_END) && !parser_check(parser, TOKEN_EOF)) {
        template_part part;
        
        if (parser_match(parser, TOKEN_TEMPLATE_TEXT)) {
            // Static text segment
            part.type = TEMPLATE_PART_TEXT;
            part.as.text = token_to_string(&parser->previous);
            
        } else if (parser_match(parser, TOKEN_TEMPLATE_SIMPLE_VAR)) {
            // Simple variable interpolation: $identifier
            char* var_str = token_to_string(&parser->previous);
            // Skip the '$' character
            char* var_name = strdup(var_str + 1);
            free(var_str);
            
            part.type = TEMPLATE_PART_EXPRESSION;
            part.as.expression = (ast_node*)ast_create_identifier(var_name, 
                                                                 parser->previous.line, 
                                                                 parser->previous.column);
            free(var_name);
            
        } else if (parser_match(parser, TOKEN_TEMPLATE_EXPR_START)) {
            // Complex expression interpolation: ${expr}
            part.type = TEMPLATE_PART_EXPRESSION;
            part.as.expression = parse_expression(parser);
            parser_consume(parser, TOKEN_TEMPLATE_EXPR_END, "Expected '}' after template expression");
            
        } else {
            parser_error_at_current(parser, "Unexpected token in template literal");
            break;
        }
        
        // Grow parts array if needed
        if (part_count >= part_capacity) {
            size_t new_capacity = part_capacity == 0 ? 8 : part_capacity * 2;
            parts = realloc(parts, sizeof(template_part) * new_capacity);
            part_capacity = new_capacity;
        }
        
        parts[part_count++] = part;
    }
    
    parser_consume(parser, TOKEN_TEMPLATE_END, "Expected closing backtick for template literal");
    
    return (ast_node*)ast_create_template_literal(parts, part_count, line, column);
}