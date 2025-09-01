#include "parser_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// Forward declarations for static functions
static const char* get_source_line(const char* source, int line_number, size_t* line_length);
static void print_error_caret(const char* source, token_t* token);

// Helper function to extract string from token
char* token_to_string(token_t* token) {
    if (!token || !token->start) return NULL;
    
    char* str = malloc(token->length + 1);
    if (!str) return NULL;
    
    memcpy(str, token->start, token->length);
    str[token->length] = '\0';
    
    return str;
}

// Helper function to extract number from token
double token_to_number(token_t* token) {
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
    parser->mode = PARSER_MODE_STRICT;  // Default to strict mode
    parser->pushback_count = 0;  // Initialize pushback
    
    // Prime the parser with the first token
    parser_advance(parser);
}

void parser_set_mode(parser_t* parser, parser_mode_t mode) {
    parser->mode = mode;
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
    
    // Check if we have pushed back tokens
    if (parser->pushback_count > 0) {
        parser->current = parser->pushed_back[parser->pushback_count - 1];
        parser->pushback_count--;
        return;
    }
    
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

void parser_pushback(parser_t* parser) {
    // Push the current token back (support multiple pushbacks)
    if (parser->pushback_count < 2) {
        parser->pushed_back[parser->pushback_count] = parser->current;
        parser->pushback_count++;
    }
    parser->current = parser->previous;
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