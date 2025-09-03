#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

// Token types for the Slate language
typedef enum {
    // Literals
    TOKEN_INTEGER,       // 123 (no decimal point)
    TOKEN_FLOAT32,       // 3.14f (single precision)
    TOKEN_FLOAT64,       // 3.14d (double precision)  
    TOKEN_NUMBER,        // 3.14 (default precision, configurable)
    TOKEN_STRING,        // "hello"
    TOKEN_IDENTIFIER,    // variable names
    TOKEN_TRUE,          // true
    TOKEN_FALSE,         // false
    TOKEN_NULL,          // null
    TOKEN_UNDEFINED,     // undefined
    TOKEN_NAN,           // NaN
    TOKEN_INFINITY,      // Infinity
    
    // Keywords
    TOKEN_VAR,           // var
    TOKEN_VAL,           // val
    TOKEN_DEF,           // def
    TOKEN_FUNCTION,      // function
    TOKEN_IF,            // if
    TOKEN_ELIF,          // elif
    TOKEN_ELSE,          // else
    TOKEN_FOR,           // for
    TOKEN_WHILE,         // while
    TOKEN_LOOP,          // loop
    TOKEN_DO,            // do
    TOKEN_BREAK,         // break
    TOKEN_CONTINUE,      // continue
    TOKEN_RETURN,        // return
    TOKEN_THEN,          // then
    TOKEN_END,           // end
    TOKEN_AND,           // and (synonym for &&)
    TOKEN_OR,            // or (synonym for ||)
    TOKEN_NOT,           // not (synonym for !)
    TOKEN_IN,            // in (property existence)
    TOKEN_INSTANCEOF,    // instanceof (type checking)
    TOKEN_IMPORT,        // import (module imports)
    TOKEN_PACKAGE,       // package (package declaration)
    TOKEN_PRIVATE,       // private (private members)
    
    // Operators
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_MULTIPLY,      // *
    TOKEN_DIVIDE,        // /
    TOKEN_MOD,           // mod
    TOKEN_POWER,         // **
    TOKEN_ASSIGN,        // =
    TOKEN_PLUS_ASSIGN,   // +=
    TOKEN_MINUS_ASSIGN,  // -=
    TOKEN_MULT_ASSIGN,   // *=
    TOKEN_DIV_ASSIGN,    // /=
    TOKEN_MOD_ASSIGN,    // %=
    TOKEN_POWER_ASSIGN,  // **=
    TOKEN_BITWISE_AND_ASSIGN,   // &=
    TOKEN_BITWISE_OR_ASSIGN,    // |=
    TOKEN_BITWISE_XOR_ASSIGN,   // ^=
    TOKEN_LEFT_SHIFT_ASSIGN,    // <<=
    TOKEN_RIGHT_SHIFT_ASSIGN,   // >>=
    TOKEN_LOGICAL_RIGHT_SHIFT_ASSIGN, // >>>=
    TOKEN_LOGICAL_AND_ASSIGN,   // &&=
    TOKEN_LOGICAL_OR_ASSIGN,    // ||=
    TOKEN_NULL_COALESCE_ASSIGN, // ??=
    TOKEN_EQUAL,         // ==
    TOKEN_NOT_EQUAL,     // !=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LOGICAL_AND,   // &&
    TOKEN_LOGICAL_OR,    // ||
    TOKEN_LOGICAL_NOT,   // !
    TOKEN_BITWISE_AND,   // &
    TOKEN_BITWISE_OR,    // |
    TOKEN_BITWISE_XOR,   // ^
    TOKEN_BITWISE_NOT,   // ~
    TOKEN_LEFT_SHIFT,    // <<
    TOKEN_RIGHT_SHIFT,   // >> (arithmetic, sign-extending)
    TOKEN_LOGICAL_RIGHT_SHIFT, // >>> (logical, zero-filling)
    TOKEN_INCREMENT,     // ++
    TOKEN_DECREMENT,     // --
    TOKEN_FLOOR_DIV,     // //
    TOKEN_RANGE,         // .. (inclusive range)
    TOKEN_RANGE_EXCLUSIVE, // ..< (exclusive range)
    TOKEN_NULL_COALESCE, // ??
    TOKEN_OPTIONAL_CHAIN, // ?.
    TOKEN_QUESTION,      // ? (for ternary)
    
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
    TOKEN_FAT_ARROW,     // =>
    
    // Template literals
    TOKEN_TEMPLATE_START,      // ` (opening backtick)
    TOKEN_TEMPLATE_TEXT,       // literal text segments
    TOKEN_TEMPLATE_SIMPLE_VAR, // $identifier
    TOKEN_TEMPLATE_EXPR_START, // ${
    TOKEN_TEMPLATE_EXPR_END,   // } (when closing expression)
    TOKEN_TEMPLATE_END,        // ` (closing backtick)
    
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

// Template lexer modes
typedef enum {
    LEXER_NORMAL,         // Regular lexing mode
    LEXER_TEMPLATE,       // Inside template literal (collecting text)
    LEXER_TEMPLATE_EXPR   // Inside ${...} expression  
} lexer_mode_t;

// Template lexer state for stack-based parsing
typedef struct {
    lexer_mode_t mode;    // Current lexing mode
    int brace_depth;      // Track {} nesting depth in expressions
} template_lexer_state;

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
    
    // Template literal state stack
    template_lexer_state* template_stack;    // Stack of template parsing states
    int template_stack_depth;               // Current depth of template stack
    int template_stack_capacity;            // Capacity of template stack
} lexer_t;

// Lexer functions
void lexer_init(lexer_t* lexer, const char* source);
void lexer_cleanup(lexer_t* lexer);
token_t lexer_next_token(lexer_t* lexer);
const char* token_type_name(token_type_t type);

// Template state management functions
void lexer_push_template_state(lexer_t* lexer, lexer_mode_t mode, int brace_depth);
void lexer_pop_template_state(lexer_t* lexer);
lexer_mode_t lexer_current_mode(lexer_t* lexer);
int lexer_current_brace_depth(lexer_t* lexer);

// Helper functions
int is_digit(char c);
int is_alpha(char c);
int is_alnum(char c);

#endif // LEXER_H