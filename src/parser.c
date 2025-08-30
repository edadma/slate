#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// Forward declarations for parser functions
static ast_node* parse_bitwise_or(parser_t* parser);
static ast_node* parse_bitwise_xor(parser_t* parser);
static ast_node* parse_bitwise_and(parser_t* parser);
static ast_node* parse_shift(parser_t* parser);
static ast_node* parse_arrow_function(parser_t* parser, char** parameters, size_t param_count);
static ast_node* parse_parenthesized_or_arrow(parser_t* parser);
static ast_node* parse_def_declaration(parser_t* parser);

// Helper function to extract string from token
static char* token_to_string(token_t* token) {
    if (!token || !token->start) return NULL;
    
    char* str = malloc(token->length + 1);
    if (!str) return NULL;
    
    memcpy(str, token->start, token->length);
    str[token->length] = '\0';
    
    return str;
}

// Helper function to extract number from token
static double token_to_number(token_t* token) {
    if (!token || !token->start) return 0.0;
    
    char* str = token_to_string(token);
    if (!str) return 0.0;
    
    double value = strtod(str, NULL);
    free(str);
    
    return value;
}

// Parser initialization
void parser_init(parser_t* parser, lexer_t* lexer) {
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    
    // Prime the parser with the first token
    parser_advance(parser);
}

// Helper function to get a specific line from source code
static const char* get_source_line(const char* source, int line_number, size_t* line_length) {
    const char* current = source;
    int current_line = 1;
    const char* line_start = source;
    
    // Find the start of the target line
    while (*current && current_line < line_number) {
        if (*current == '\n') {
            current_line++;
            line_start = current + 1;
        }
        current++;
    }
    
    if (current_line != line_number) {
        *line_length = 0;
        return NULL;
    }
    
    // Find the end of the line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }
    
    *line_length = line_end - line_start;
    return line_start;
}

// Helper function to print caret pointing to error position
static void print_error_caret(const char* source, token_t* token) {
    size_t line_length;
    const char* line_start = get_source_line(source, token->line, &line_length);
    
    if (!line_start) return;
    
    // Print the source line
    fprintf(stderr, "    %.*s\n", (int)line_length, line_start);
    
    // Calculate the column position of the token within the line
    int caret_position = (int)(token->start - line_start);
    
    // Print spaces followed by caret
    fprintf(stderr, "    ");
    for (int i = 0; i < caret_position; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
}

// Error handling
void parser_error_at(parser_t* parser, token_t* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = 1;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // For error tokens, show the problematic character
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    } else {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    
    // Print source line with caret if we have access to source
    if (parser->lexer && parser->lexer->source) {
        print_error_caret(parser->lexer->source, token);
    }
    
    parser->had_error = 1;
}

void parser_error(parser_t* parser, const char* message) {
    parser_error_at(parser, &parser->previous, message);
}

void parser_error_at_current(parser_t* parser, const char* message) {
    parser_error_at(parser, &parser->current, message);
}

// Token consumption
void parser_advance(parser_t* parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        
        parser_error_at_current(parser, "Unexpected character");
    }
}

int parser_check(parser_t* parser, token_type_t type) {
    return parser->current.type == type;
}

int parser_match(parser_t* parser, token_type_t type) {
    if (!parser_check(parser, type)) return 0;
    parser_advance(parser);
    return 1;
}

void parser_consume(parser_t* parser, token_type_t type, const char* message) {
    if (parser->current.type == type) {
        parser_advance(parser);
        return;
    }
    
    parser_error_at_current(parser, message);
}

void parser_synchronize(parser_t* parser) {
    parser->panic_mode = 0;
    
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;
        if (parser->previous.type == TOKEN_NEWLINE) return;
        
        switch (parser->current.type) {
            case TOKEN_VAR:
            case TOKEN_FUNCTION:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                // Do nothing
                break;
        }
        
        parser_advance(parser);
    }
}

// Binary operator conversion
binary_operator token_to_binary_op(token_type_t type) {
    switch (type) {
        case TOKEN_PLUS: return BIN_ADD;
        case TOKEN_MINUS: return BIN_SUBTRACT;
        case TOKEN_MULTIPLY: return BIN_MULTIPLY;
        case TOKEN_DIVIDE: return BIN_DIVIDE;
        case TOKEN_MOD: return BIN_MOD;
        case TOKEN_POWER: return BIN_POWER;
        case TOKEN_EQUAL: return BIN_EQUAL;
        case TOKEN_NOT_EQUAL: return BIN_NOT_EQUAL;
        case TOKEN_LESS: return BIN_LESS;
        case TOKEN_LESS_EQUAL: return BIN_LESS_EQUAL;
        case TOKEN_GREATER: return BIN_GREATER;
        case TOKEN_GREATER_EQUAL: return BIN_GREATER_EQUAL;
        case TOKEN_LOGICAL_AND: return BIN_LOGICAL_AND;
        case TOKEN_AND: return BIN_LOGICAL_AND;
        case TOKEN_LOGICAL_OR: return BIN_LOGICAL_OR;
        case TOKEN_OR: return BIN_LOGICAL_OR;
        case TOKEN_BITWISE_AND: return BIN_BITWISE_AND;
        case TOKEN_BITWISE_OR: return BIN_BITWISE_OR;
        case TOKEN_BITWISE_XOR: return BIN_BITWISE_XOR;
        case TOKEN_LEFT_SHIFT: return BIN_LEFT_SHIFT;
        case TOKEN_RIGHT_SHIFT: return BIN_RIGHT_SHIFT;
        case TOKEN_LOGICAL_RIGHT_SHIFT: return BIN_LOGICAL_RIGHT_SHIFT;
        case TOKEN_FLOOR_DIV: return BIN_FLOOR_DIV;
        case TOKEN_NULL_COALESCE: return BIN_NULL_COALESCE;
        case TOKEN_IN: return BIN_IN;
        case TOKEN_INSTANCEOF: return BIN_INSTANCEOF;
        default: return BIN_ADD; // Fallback
    }
}

// Unary operator conversion
unary_operator token_to_unary_op(token_type_t type) {
    switch (type) {
        case TOKEN_MINUS: return UN_NEGATE;
        case TOKEN_LOGICAL_NOT: return UN_NOT;
        case TOKEN_NOT: return UN_NOT;
        case TOKEN_BITWISE_NOT: return UN_BITWISE_NOT;
        case TOKEN_INCREMENT: return UN_PRE_INCREMENT;
        case TOKEN_DECREMENT: return UN_PRE_DECREMENT;
        default: return UN_NEGATE; // Fallback
    }
}

// Parse program (entry point)
ast_program* parse_program(parser_t* parser) {
    ast_node** statements = NULL;
    size_t statement_count = 0;
    size_t statement_capacity = 0;
    
    // Skip any initial newlines
    while (parser_match(parser, TOKEN_NEWLINE));
    
    while (!parser_check(parser, TOKEN_EOF)) {
        ast_node* stmt = parse_declaration(parser);
        
        if (stmt) {
            // Grow statements array if needed
            if (statement_count >= statement_capacity) {
                size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
                statements = realloc(statements, sizeof(ast_node*) * new_capacity);
                statement_capacity = new_capacity;
            }
            
            statements[statement_count++] = stmt;
        }
        
        if (parser->panic_mode) parser_synchronize(parser);
        
        // Skip newlines between statements
        while (parser_match(parser, TOKEN_NEWLINE));
    }
    
    return ast_create_program(statements, statement_count, 
                             parser->previous.line, parser->previous.column);
}

// Parse declaration
ast_node* parse_declaration(parser_t* parser) {
    if (parser_match(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser);
    }
    
    if (parser_match(parser, TOKEN_DEF)) {
        return parse_def_declaration(parser);
    }
    
    return parse_statement(parser);
}

// Parse variable declaration
ast_node* parse_var_declaration(parser_t* parser) {
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
    
    char* name = token_to_string(&parser->previous);
    ast_node* initializer = NULL;
    
    if (parser_match(parser, TOKEN_ASSIGN)) {
        initializer = parse_expression(parser);
    }
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_var_declaration(name, initializer, 
                                                parser->previous.line, parser->previous.column);
}

// Parse def function declaration: def name(params) = expr or def name(params) =\n<indent>
ast_node* parse_def_declaration(parser_t* parser) {
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected function name after 'def'");
    
    char* func_name = token_to_string(&parser->previous);
    int name_line = parser->previous.line;
    int name_column = parser->previous.column;
    
    // Parse parameter list
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");
    
    char** parameters = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            char* param = token_to_string(&parser->previous);
            
            // Grow parameters array
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                parameters = realloc(parameters, param_capacity * sizeof(char*));
            }
            parameters[param_count++] = param;
            
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    parser_consume(parser, TOKEN_ASSIGN, "Expected '=' after parameter list");
    
    // Parse function body - support both single-line and indented block forms
    ast_node* body = NULL;
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Indented block form: def name(params) =\n  <block>
        body = parse_indented_block(parser);
    } else {
        // Single-line form: def name(params) = expr
        body = parse_expression(parser);
    }
    
    // Create function AST node (both forms are treated as expressions)
    ast_node* func_node = (ast_node*)ast_create_function(parameters, param_count, body, 1,
                                                        name_line, name_column);
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    // Return this as a variable declaration with the function as initializer
    return (ast_node*)ast_create_var_declaration(func_name, func_node, name_line, name_column);
}

// Parse statement
ast_node* parse_statement(parser_t* parser) {
    if (parser_match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_LOOP)) {
        return parse_loop_statement(parser);
    }
    
    
    if (parser_match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }
    
    return parse_expression_statement(parser);
}

// Parse indented block of statements
ast_node* parse_indented_block(parser_t* parser) {
    // Skip optional newline before indent
    parser_match(parser, TOKEN_NEWLINE);
    
    if (!parser_match(parser, TOKEN_INDENT)) {
        parser_error_at_current(parser, "Expected indented block.");
        return NULL;
    }
    
    // Parse statements until we hit a DEDENT
    ast_node** statements = NULL;
    size_t statement_count = 0;
    size_t statement_capacity = 0;
    
    while (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF)) {
        // Skip newlines between statements
        while (parser_match(parser, TOKEN_NEWLINE));
        
        if (parser_check(parser, TOKEN_DEDENT)) break;
        
        ast_node* stmt = parse_declaration(parser);
        
        if (statement_count >= statement_capacity) {
            size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
            statements = realloc(statements, sizeof(ast_node*) * new_capacity);
            statement_capacity = new_capacity;
        }
        
        statements[statement_count++] = stmt;
        
        // Skip optional trailing newlines
        while (parser_match(parser, TOKEN_NEWLINE));
    }
    
    parser_consume(parser, TOKEN_DEDENT, "Expected dedent after block.");
    
    // Validate that the block ends with an expression
    if (statement_count == 0) {
        // Empty block - treat as null
        return (ast_node*)ast_create_null(parser->previous.line, parser->previous.column);
    } else {
        ast_node* last_stmt = statements[statement_count - 1];
        if (last_stmt->type != AST_EXPRESSION_STMT) {
            parser_error(parser, "Block expressions must end with an expression, not a statement");
            return NULL;
        }
        
        // If the last expression is a block, validate it recursively
        ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
        if (!validate_block_expression(expr_stmt->expression)) {
            parser_error(parser, "Nested block expressions must ultimately end with a non-block expression");
            return NULL;
        }
    }
    
    return (ast_node*)ast_create_block(statements, statement_count,
                                      parser->previous.line, parser->previous.column);
}

// Parse if expression with new syntax
ast_node* parse_if_expression(parser_t* parser) {
    // Parse condition (no parentheses required)
    ast_node* condition = parse_expression(parser);
    
    ast_node* then_expr = NULL;
    ast_node* else_expr = NULL;
    
    // Check for 'then' keyword
    if (parser_match(parser, TOKEN_THEN)) {
        // Could be: 'if condition then expression' or 'if condition then\n<indent>'
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line form with 'then'
            then_expr = parse_indented_block(parser);
        } else {
            // Single-line form with 'then'
            then_expr = parse_expression(parser);
        }
    } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form without 'then'
        then_expr = parse_indented_block(parser);
    } else {
        parser_error_at_current(parser, "Expected 'then' or indented block after if condition.");
        return NULL;
    }
    
    // Skip optional newlines before checking for 'else'
    while (parser_match(parser, TOKEN_NEWLINE));
    
    // Check for 'else' clause
    if (parser_match(parser, TOKEN_ELSE)) {
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line else
            else_expr = parse_indented_block(parser);
        } else {
            // Single-line else
            else_expr = parse_expression(parser);
        }
    }
    
    // Check for optional 'end if'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_IF, "Expected 'if' after 'end'.");
    }
    
    return (ast_node*)ast_create_if(condition, then_expr, else_expr,
                                   parser->previous.line, parser->previous.column);
}

// Validate that a block expression ultimately ends with a non-block expression
int validate_block_expression(ast_node* expr) {
    if (expr->type != AST_BLOCK) {
        // Non-block expression is valid
        return 1;
    }
    
    // It's a block, check its contents
    ast_block* block = (ast_block*)expr;
    
    if (block->statement_count == 0) {
        // Empty block is valid
        return 1;
    }
    
    // Last statement must be an expression statement
    ast_node* last_stmt = block->statements[block->statement_count - 1];
    if (last_stmt->type != AST_EXPRESSION_STMT) {
        return 0; // Block ends with statement, invalid
    }
    
    // Recursively validate the last expression
    ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
    return validate_block_expression(expr_stmt->expression);
}


// Parse while statement with new syntax
ast_node* parse_while_statement(parser_t* parser) {
    // Parse condition (no parentheses required)
    ast_node* condition = parse_expression(parser);
    
    ast_node* body = NULL;
    
    // Check for 'do' keyword
    if (parser_match(parser, TOKEN_DO)) {
        // Could be: 'while condition do expression' or 'while condition do\n<indent>'
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line form with 'do'
            body = parse_indented_block(parser);
        } else {
            // Single-line form with 'do'
            body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                         parser->current.line, parser->current.column);
        }
    } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form without 'do'
        body = parse_indented_block(parser);
    } else {
        parser_error_at_current(parser, "Expected 'do' or indented block after while condition.");
        return NULL;
    }
    
    // Check for optional 'end while'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_WHILE, "Expected 'while' after 'end'.");
    }
    
    return (ast_node*)ast_create_while(condition, body,
                                      parser->previous.line, parser->previous.column);
}

// Parse infinite loop statement
ast_node* parse_loop_statement(parser_t* parser) {
    ast_node* body = NULL;
    
    // Parse the loop body - supports both single-line and multi-line forms
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form with indented block
        body = parse_indented_block(parser);
    } else {
        // Single-line form: loop <expression>
        body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                     parser->current.line, parser->current.column);
    }
    
    // Check for optional 'end loop'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_LOOP, "Expected 'loop' after 'end'.");
    }
    
    return (ast_node*)ast_create_loop(body, parser->previous.line, parser->previous.column);
}

// Parse break statement
ast_node* parse_break_statement(parser_t* parser) {
    // Break statements are simple - just the keyword
    return (ast_node*)ast_create_break(parser->previous.line, parser->previous.column);
}

// Parse continue statement
ast_node* parse_continue_statement(parser_t* parser) {
    // Continue statements are simple - just the keyword
    return (ast_node*)ast_create_continue(parser->previous.line, parser->previous.column);
}

// Parse return statement
ast_node* parse_return_statement(parser_t* parser) {
    ast_node* value = NULL;
    
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE)) {
        value = parse_expression(parser);
    }
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_return(value, parser->previous.line, parser->previous.column);
}


// Parse expression statement
ast_node* parse_expression_statement(parser_t* parser) {
    ast_node* expr = parse_expression(parser);
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_expression_stmt(expr, parser->previous.line, parser->previous.column);
}

// Parse expression
ast_node* parse_expression(parser_t* parser) {
    return parse_assignment(parser);
}

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
        expr = (ast_node*)ast_create_range(expr, end, exclusive, op_line, op_column);
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

// Parse factor (* /)
static ast_node* parse_power(parser_t* parser);
static ast_node* parse_postfix(parser_t* parser);

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
            expr = (ast_node*)ast_create_member(expr, name, 
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
        char* name = token_to_string(&parser->previous);
        ast_node* result = (ast_node*)ast_create_identifier(name, parser->previous.line, parser->previous.column);
        free(name);
        return result;
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
    
    if (parser_match(parser, TOKEN_BREAK)) {
        return parse_break_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_CONTINUE)) {
        return parse_continue_statement(parser);
    }
    
    parser_error_at_current(parser, "Expected expression.");
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
    
    // First, try to parse as parameter list
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        do {
            if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                // Not a parameter list, must be grouped expression
                // But we need to backtrack - this is complex
                // For now, let's implement a simpler approach
                parser_error(parser, "Expected parameter name");
                return NULL;
            }
            
            char* param = token_to_string(&parser->previous);
            
            // Grow parameters array
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                parameters = realloc(parameters, param_capacity * sizeof(char*));
            }
            parameters[param_count++] = param;
            
        } while (parser_match(parser, TOKEN_COMMA));
        
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
        
        if (parser_check(parser, TOKEN_ARROW)) {
            // This is definitely an arrow function
            return parse_arrow_function(parser, parameters, param_count);
        } else {
            // This looked like parameters but no arrow - error
            // Clean up allocated parameters
            for (size_t i = 0; i < param_count; i++) {
                free(parameters[i]);
            }
            free(parameters);
            parser_error(parser, "Expected '->' after parameter list");
            return NULL;
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