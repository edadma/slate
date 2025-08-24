#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#include <stddef.h>

#define INPUT_BUFFER_SIZE 1024
#define HISTORY_SIZE 50

typedef enum {
    KEY_NORMAL,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_ENTER,
    KEY_CTRL_LEFT,
    KEY_CTRL_RIGHT,
    KEY_CTRL_DELETE,
    KEY_ALT_BACKSPACE
} key_type_t;

typedef struct {
    key_type_t type;
    char character;  // Only valid when type == KEY_NORMAL
} key_event_t;

typedef struct {
    char buffer[INPUT_BUFFER_SIZE];
    size_t length;
    size_t cursor_pos;
} line_buffer_t;

typedef struct {
    char history[HISTORY_SIZE][INPUT_BUFFER_SIZE];
    int current_entry;  // Index of most recent entry
    int total_entries;  // Total number of entries (up to HISTORY_SIZE)
    int viewing_entry;  // Currently viewing entry (-1 = current line)
} history_buffer_t;

// Platform-independent line editing - main entry point
void get_line_with_editing(char* buffer, size_t max_len);

// History management
void history_init(history_buffer_t* hist);
void history_add(history_buffer_t* hist, const char* line);
const char* history_get_previous(history_buffer_t* hist);
const char* history_get_next(history_buffer_t* hist);
void history_reset_view(history_buffer_t* hist);

// Terminal control functions
void terminal_raw_mode_enter(void);
void terminal_raw_mode_exit(void);
void terminal_clear_eol(void);
void terminal_cursor_left(void);
void terminal_cursor_right(void);

// Key parsing
key_event_t parse_key_sequence(void);

#endif  // LINE_EDITOR_H