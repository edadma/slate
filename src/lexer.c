#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Helper functions
int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

// Keywords table
typedef struct {
    const char* keyword;
    token_type_t type;
} keyword_entry_t;

static keyword_entry_t keywords[] = {
    {"var", TOKEN_VAR},
    {"def", TOKEN_DEF},
    {"function", TOKEN_FUNCTION},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"loop", TOKEN_LOOP},
    {"do", TOKEN_DO},
    {"return", TOKEN_RETURN},
    {"then", TOKEN_THEN},
    {"end", TOKEN_END},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    {"mod", TOKEN_MOD},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"null", TOKEN_NULL},
    {"undefined", TOKEN_UNDEFINED},
    {NULL, TOKEN_EOF} // Sentinel
};

static token_type_t check_keyword(const char* start, size_t length) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strlen(keywords[i].keyword) == length && 
            memcmp(start, keywords[i].keyword, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

// Lexer implementation
void lexer_init(lexer_t* lexer, const char* source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    
    // Initialize indentation tracking
    lexer->indent_capacity = 8;
    lexer->indent_stack = malloc(sizeof(int) * lexer->indent_capacity);
    lexer->indent_stack[0] = 0;  // Base indentation level is 0
    lexer->indent_count = 1;
    lexer->pending_indent = 0;
    lexer->at_line_start = 1;  // We start at the beginning of a line
    lexer->pending_dedents = 0;
    lexer->brace_depth = 0;  // Not inside any braces initially
}

void lexer_cleanup(lexer_t* lexer) {
    if (lexer->indent_stack) {
        free(lexer->indent_stack);
        lexer->indent_stack = NULL;
    }
}

static int is_at_end(lexer_t* lexer) {
    return *lexer->current == '\0';
}

static char advance(lexer_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    
    char c = *lexer->current;
    lexer->current++;
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

static char peek(lexer_t* lexer) {
    return *lexer->current;
}

static char peek_next(lexer_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static int match(lexer_t* lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (*lexer->current != expected) return 0;
    
    advance(lexer);
    return 1;
}

static token_t make_token(lexer_t* lexer, token_type_t type) {
    token_t token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->column - (int)token.length;
    return token;
}

static token_t error_token(lexer_t* lexer, const char* message) {
    token_t token;
    token.type = TOKEN_ERROR;
    // For error tokens, we want to point to the previous character (the bad one)
    token.start = lexer->current - 1;
    token.length = 1; // Point to the bad character
    token.line = lexer->line;
    token.column = lexer->column - 1;
    return token;
}

static void skip_whitespace(lexer_t* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\\':
                // Forth-style line comment - skip until end of line
                while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                    advance(lexer);
                }
                break;
            case '/':
                if (peek_next(lexer) == '*') {
                    // Block comment - skip until */
                    advance(lexer); // Skip '/'
                    advance(lexer); // Skip '*'
                    
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); // Skip '*'
                            advance(lexer); // Skip '/'
                            break;
                        }
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static token_t string_token(lexer_t* lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0; // Will be incremented by advance()
        }
        
        // Handle escape sequences
        if (peek(lexer) == '\\') {
            advance(lexer); // Consume the backslash
            if (!is_at_end(lexer)) {
                // Consume the escaped character (whatever it is)
                if (peek(lexer) == '\n') {
                    lexer->line++;
                    lexer->column = 0;
                }
                advance(lexer);
            }
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }
    
    // Closing quote
    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}

static token_t number_token(lexer_t* lexer) {
    int is_float = 0;
    
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }
    
    // Look for decimal point
    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        is_float = 1;
        // Consume the '.'
        advance(lexer);
        
        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    // Look for exponent notation (e.g., 1e10, 2.5e-3)
    if ((peek(lexer) == 'e' || peek(lexer) == 'E')) {
        char next = peek_next(lexer);
        if (is_digit(next) || (next == '+' || next == '-')) {
            is_float = 1;
            advance(lexer); // Consume 'e' or 'E'
            
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer); // Consume sign
            }
            
            while (is_digit(peek(lexer))) {
                advance(lexer);
            }
        }
    }
    
    return make_token(lexer, is_float ? TOKEN_NUMBER : TOKEN_INTEGER);
}

static token_t identifier_token(lexer_t* lexer) {
    while (is_alnum(peek(lexer))) {
        advance(lexer);
    }
    
    token_type_t type = check_keyword(lexer->start, 
                                    (size_t)(lexer->current - lexer->start));
    return make_token(lexer, type);
}

token_t lexer_next_token(lexer_t* lexer) {
    // First, check if we need to emit pending DEDENT tokens
    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;
        return make_token(lexer, TOKEN_DEDENT);
    }
    
    // If we're at the start of a line AND not inside braces, handle indentation
    if (lexer->at_line_start && lexer->brace_depth == 0) {
        lexer->at_line_start = 0;
        
        // Count spaces at the beginning of the line
        int spaces = 0;
        while (peek(lexer) == ' ') {
            spaces++;
            advance(lexer);
        }
        
        // Skip blank lines and lines with only comments
        if (peek(lexer) == '\n' || 
            peek(lexer) == '\\' ||
            (peek(lexer) == '/' && (peek_next(lexer) == '/' || peek_next(lexer) == '*'))) {
            skip_whitespace(lexer);
            if (peek(lexer) == '\n') {
                advance(lexer);
                lexer->at_line_start = 1;
                return lexer_next_token(lexer); // Recursively skip blank lines
            }
        }
        
        // Not a blank line, check indentation level
        int current_indent = lexer->indent_stack[lexer->indent_count - 1];
        
        if (spaces > current_indent) {
            // Increase indentation - push new level
            if (lexer->indent_count >= lexer->indent_capacity) {
                lexer->indent_capacity *= 2;
                lexer->indent_stack = realloc(lexer->indent_stack, 
                                            sizeof(int) * lexer->indent_capacity);
            }
            lexer->indent_stack[lexer->indent_count++] = spaces;
            lexer->start = lexer->current;
            return make_token(lexer, TOKEN_INDENT);
        } else if (spaces < current_indent) {
            // Decrease indentation - pop levels and emit DEDENTs
            while (lexer->indent_count > 1 && 
                   lexer->indent_stack[lexer->indent_count - 1] > spaces) {
                lexer->indent_count--;
                lexer->pending_dedents++;
            }
            
            // Check that we dedented to a valid level
            if (lexer->indent_stack[lexer->indent_count - 1] != spaces) {
                return error_token(lexer, "Inconsistent indentation");
            }
            
            // Emit the first DEDENT, others are pending
            if (lexer->pending_dedents > 0) {
                lexer->pending_dedents--;
                lexer->start = lexer->current;
                return make_token(lexer, TOKEN_DEDENT);
            }
        }
        // Same indentation level - continue normally
    }
    
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        // Emit any remaining DEDENTs at EOF
        if (lexer->indent_count > 1) {
            lexer->indent_count--;
            return make_token(lexer, TOKEN_DEDENT);
        }
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    if (is_digit(c)) return number_token(lexer);
    if (is_alpha(c)) return identifier_token(lexer);
    
    switch (c) {
        case '(': 
            lexer->brace_depth++;
            return make_token(lexer, TOKEN_LEFT_PAREN);
        case ')': 
            lexer->brace_depth--;
            return make_token(lexer, TOKEN_RIGHT_PAREN);
        case '{': 
            lexer->brace_depth++;
            return make_token(lexer, TOKEN_LEFT_BRACE);
        case '}': 
            lexer->brace_depth--;
            return make_token(lexer, TOKEN_RIGHT_BRACE);
        case '[': 
            lexer->brace_depth++;
            return make_token(lexer, TOKEN_LEFT_BRACKET);
        case ']': 
            lexer->brace_depth--;
            return make_token(lexer, TOKEN_RIGHT_BRACKET);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case ':': return make_token(lexer, TOKEN_COLON);
        case '.':
            if (match(lexer, '.')) {
                // We have ".."
                if (match(lexer, '<')) {
                    // We have "..<" (exclusive range)
                    return make_token(lexer, TOKEN_RANGE_EXCLUSIVE);
                }
                // We have ".." (inclusive range)
                return make_token(lexer, TOKEN_RANGE);
            }
            return make_token(lexer, TOKEN_DOT);
        case '+': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_PLUS_ASSIGN);
            } else if (match(lexer, '+')) {
                return make_token(lexer, TOKEN_INCREMENT);
            }
            return make_token(lexer, TOKEN_PLUS);
        case '*': 
            if (peek(lexer) == '*') {
                advance(lexer);
                if (match(lexer, '=')) {
                    return make_token(lexer, TOKEN_POWER_ASSIGN);
                }
                return make_token(lexer, TOKEN_POWER);
            }
            return make_token(lexer, match(lexer, '=') ? TOKEN_MULT_ASSIGN : TOKEN_MULTIPLY);
        case '/': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_DIV_ASSIGN);
            } else if (match(lexer, '/')) {
                return make_token(lexer, TOKEN_FLOOR_DIV);
            }
            return make_token(lexer, TOKEN_DIVIDE);
        case '%': 
            return make_token(lexer, match(lexer, '=') ? TOKEN_MOD_ASSIGN : TOKEN_MOD);
        case '\n': 
            // Only track line starts when not inside braces
            if (lexer->brace_depth == 0) {
                lexer->at_line_start = 1;
            }
            return make_token(lexer, TOKEN_NEWLINE);
        
        case '-':
            if (match(lexer, '>')) {
                return make_token(lexer, TOKEN_ARROW);
            } else if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_MINUS_ASSIGN);
            } else if (match(lexer, '-')) {
                return make_token(lexer, TOKEN_DECREMENT);
            }
            return make_token(lexer, TOKEN_MINUS);
            
        case '!':
            return make_token(lexer, match(lexer, '=') ? TOKEN_NOT_EQUAL : TOKEN_LOGICAL_NOT);
            
        case '=':
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQUAL : TOKEN_ASSIGN);
            
        case '<':
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_LESS_EQUAL);
            } else if (match(lexer, '<')) {
                return make_token(lexer, TOKEN_LEFT_SHIFT);
            }
            return make_token(lexer, TOKEN_LESS);
            
        case '>':
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_GREATER_EQUAL);
            } else if (match(lexer, '>')) {
                if (match(lexer, '>')) {
                    return make_token(lexer, TOKEN_LOGICAL_RIGHT_SHIFT);
                }
                return make_token(lexer, TOKEN_RIGHT_SHIFT);
            }
            return make_token(lexer, TOKEN_GREATER);
            
        case '&':
            if (match(lexer, '&')) {
                return make_token(lexer, TOKEN_LOGICAL_AND);
            }
            return make_token(lexer, TOKEN_BITWISE_AND);
            
        case '|':
            if (match(lexer, '|')) {
                return make_token(lexer, TOKEN_LOGICAL_OR);
            }
            return make_token(lexer, TOKEN_BITWISE_OR);
            
        case '^': return make_token(lexer, TOKEN_BITWISE_XOR);
        case '~': return make_token(lexer, TOKEN_BITWISE_NOT);
        case '"': return string_token(lexer);
    }
    
    char bad_char = advance(lexer);
    (void)bad_char; // Suppress unused variable warning
    return error_token(lexer, "Unexpected character");
}

const char* token_type_name(token_type_t type) {
    switch (type) {
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NULL: return "NULL";
        case TOKEN_UNDEFINED: return "UNDEFINED";
        case TOKEN_VAR: return "VAR";
        case TOKEN_FUNCTION: return "FUNCTION";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_LOOP: return "LOOP";
        case TOKEN_DO: return "DO";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_THEN: return "THEN";
        case TOKEN_END: return "END";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULTIPLY: return "MULTIPLY";
        case TOKEN_DIVIDE: return "DIVIDE";
        case TOKEN_MOD: return "MOD";
        case TOKEN_POWER: return "POWER";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TOKEN_MULT_ASSIGN: return "MULT_ASSIGN";
        case TOKEN_DIV_ASSIGN: return "DIV_ASSIGN";
        case TOKEN_MOD_ASSIGN: return "MOD_ASSIGN";
        case TOKEN_POWER_ASSIGN: return "POWER_ASSIGN";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_LOGICAL_AND: return "LOGICAL_AND";
        case TOKEN_LOGICAL_OR: return "LOGICAL_OR";
        case TOKEN_LOGICAL_NOT: return "LOGICAL_NOT";
        case TOKEN_BITWISE_AND: return "BITWISE_AND";
        case TOKEN_BITWISE_OR: return "BITWISE_OR";
        case TOKEN_BITWISE_XOR: return "BITWISE_XOR";
        case TOKEN_BITWISE_NOT: return "BITWISE_NOT";
        case TOKEN_LEFT_SHIFT: return "LEFT_SHIFT";
        case TOKEN_RIGHT_SHIFT: return "RIGHT_SHIFT";
        case TOKEN_LOGICAL_RIGHT_SHIFT: return "LOGICAL_RIGHT_SHIFT";
        case TOKEN_INCREMENT: return "INCREMENT";
        case TOKEN_DECREMENT: return "DECREMENT";
        case TOKEN_FLOOR_DIV: return "FLOOR_DIV";
        case TOKEN_RANGE: return "RANGE";
        case TOKEN_RANGE_EXCLUSIVE: return "RANGE_EXCLUSIVE";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_COLON: return "COLON";
        case TOKEN_DOT: return "DOT";
        case TOKEN_LEFT_PAREN: return "LEFT_PAREN";
        case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
        case TOKEN_LEFT_BRACE: return "LEFT_BRACE";
        case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
        case TOKEN_LEFT_BRACKET: return "LEFT_BRACKET";
        case TOKEN_RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_INDENT: return "INDENT";
        case TOKEN_DEDENT: return "DEDENT";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}