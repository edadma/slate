#ifndef BIT_LEXER_H
#define BIT_LEXER_H

#include <stddef.h>

// Token types for the Bitty language
typedef enum {
    // Literals
    TOKEN_NUMBER,        // 123, 3.14
    TOKEN_STRING,        // "hello"
    TOKEN_IDENTIFIER,    // variable names
    TOKEN_TRUE,          // true
    TOKEN_FALSE,         // false
    TOKEN_NULL,          // null
    TOKEN_UNDEFINED,     // undefined
    
    // Keywords
    TOKEN_VAR,           // var
    TOKEN_FUNCTION,      // function
    TOKEN_IF,            // if
    TOKEN_ELSE,          // else
    TOKEN_WHILE,         // while
    TOKEN_RETURN,        // return
    TOKEN_THEN,          // then
    TOKEN_END,           // end
    
    // Operators
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_MULTIPLY,      // *
    TOKEN_DIVIDE,        // /
    TOKEN_ASSIGN,        // =
    TOKEN_EQUAL,         // ==
    TOKEN_NOT_EQUAL,     // !=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LOGICAL_AND,   // &&
    TOKEN_LOGICAL_OR,    // ||
    TOKEN_LOGICAL_NOT,   // !
    
    // Punctuation
    TOKEN_SEMICOLON,     // ;
    TOKEN_COMMA,         // ,
    TOKEN_COLON,         // :
    TOKEN_DOT,           // .
    TOKEN_LEFT_PAREN,    // (
    TOKEN_RIGHT_PAREN,   // )
    TOKEN_LEFT_BRACE,    // {
    TOKEN_RIGHT_BRACE,   // }
    TOKEN_LEFT_BRACKET,  // [
    TOKEN_RIGHT_BRACKET, // ]
    TOKEN_ARROW,         // ->
    
    // Special
    TOKEN_NEWLINE,       // \n (for statement termination)
    TOKEN_INDENT,        // Increased indentation
    TOKEN_DEDENT,        // Decreased indentation
    TOKEN_EOF,           // End of file
    TOKEN_ERROR          // Error token
} token_type_t;

// Token structure
typedef struct {
    token_type_t type;
    const char* start;    // Points to start of token in source
    size_t length;        // Length of token
    int line;             // Line number for error reporting
    int column;           // Column number for error reporting
} token_t;

// Lexer state
typedef struct {
    const char* source;   // Source code string
    const char* start;    // Start of current token
    const char* current;  // Current character being examined
    int line;             // Current line number
    int column;           // Current column number
    
    // Indentation tracking
    int* indent_stack;    // Stack of indentation levels
    int indent_count;     // Number of indent levels
    int indent_capacity;  // Capacity of indent stack
    int pending_indent;   // Pending indentation level for next line
    int at_line_start;    // Whether we're at the start of a line
    int pending_dedents;  // Number of DEDENT tokens to emit
    int brace_depth;      // Track nesting depth of (), [], {} to ignore indentation inside
} lexer_t;

// Lexer functions
void lexer_init(lexer_t* lexer, const char* source);
void lexer_cleanup(lexer_t* lexer);
token_t lexer_next_token(lexer_t* lexer);
const char* token_type_name(token_type_t type);

// Helper functions
int is_digit(char c);
int is_alpha(char c);
int is_alnum(char c);

#endif // BIT_LEXER_H