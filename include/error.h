#pragma once
#include <setjmp.h>
#include <stdbool.h>

// Error kinds
typedef enum {
    ERR_NONE = 0,
    ERR_OOM,
    ERR_SYNTAX,
    ERR_TYPE,
    ERR_REFERENCE,
    ERR_RANGE,
    ERR_IO,
    ERR_ASSERT,
    ERR_ARITHMETIC, // For division by zero and similar
} ErrorKind;

// Captured error details
typedef struct {
    ErrorKind kind;
    const char* file;
    int line; // 1-based line number
    int column; // 1-based column number
    char message[256];
} SlateError;
