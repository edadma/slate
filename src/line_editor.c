#include "line_editor.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

// Global history buffer
static history_buffer_t command_history = {0};

// Platform-specific terminal state
#ifndef _WIN32
static struct termios original_termios;
static bool raw_mode_active = false;
#endif

// History management functions
void history_init(history_buffer_t *hist) {
    hist->current_entry = -1;
    hist->total_entries = 0;
    hist->viewing_entry = -1;
}

void history_add(history_buffer_t *hist, const char *line) {
    // Skip empty lines
    if (!line || strlen(line) == 0) {
        return;
    }

    // Skip if same as most recent entry
    if (hist->total_entries > 0) {
        int recent_idx = hist->current_entry;
        if (strcmp(hist->history[recent_idx], line) == 0) {
            return;
        }
    }

    // Add to circular buffer
    hist->current_entry = (hist->current_entry + 1) % HISTORY_SIZE;
    strncpy(hist->history[hist->current_entry], line, INPUT_BUFFER_SIZE - 1);
    hist->history[hist->current_entry][INPUT_BUFFER_SIZE - 1] = '\0';

    if (hist->total_entries < HISTORY_SIZE) {
        hist->total_entries++;
    }

    // Reset view to current
    hist->viewing_entry = -1;
}

const char *history_get_previous(history_buffer_t *hist) {
    if (hist->total_entries == 0) {
        return NULL;
    }

    if (hist->viewing_entry == -1) {
        // First time going back - start at most recent
        hist->viewing_entry = hist->current_entry;
    } else {
        // Go further back
        int prev_entry;
        if (hist->total_entries < HISTORY_SIZE) {
            // Haven't filled buffer yet
            if (hist->viewing_entry > 0) {
                prev_entry = hist->viewing_entry - 1;
            } else {
                return NULL;  // At oldest entry
            }
        } else {
            // Buffer is full, wrap around
            prev_entry = (hist->viewing_entry - 1 + HISTORY_SIZE) % HISTORY_SIZE;
            if (prev_entry == hist->current_entry) {
                return NULL;  // Back to newest, don't wrap
            }
        }
        hist->viewing_entry = prev_entry;
    }

    return hist->history[hist->viewing_entry];
}

const char *history_get_next(history_buffer_t *hist) {
    if (hist->viewing_entry == -1) {
        return NULL;  // Already at current line
    }

    int next_entry;
    if (hist->total_entries < HISTORY_SIZE) {
        // Haven't filled buffer yet
        if (hist->viewing_entry < hist->current_entry) {
            next_entry = hist->viewing_entry + 1;
        } else {
            hist->viewing_entry = -1;
            return NULL;  // Back to current line
        }
    } else {
        // Buffer is full
        next_entry = (hist->viewing_entry + 1) % HISTORY_SIZE;
        if (next_entry == (hist->current_entry + 1) % HISTORY_SIZE) {
            hist->viewing_entry = -1;
            return NULL;  // Back to current line
        }
    }

    hist->viewing_entry = next_entry;
    return hist->history[hist->viewing_entry];
}

void history_reset_view(history_buffer_t *hist) { 
    hist->viewing_entry = -1; 
}

// Terminal control - Platform specific implementations
#ifdef _WIN32

void terminal_raw_mode_enter(void) {
    // Windows console setup would go here
    // For now, we'll use simpler approach
}

void terminal_raw_mode_exit(void) {
    // Windows console cleanup would go here
}

void terminal_clear_eol(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    
    COORD coord = csbi.dwCursorPosition;
    DWORD count = csbi.dwSize.X - coord.X;
    DWORD written;
    
    FillConsoleOutputCharacter(hStdOut, ' ', count, coord, &written);
    SetConsoleCursorPosition(hStdOut, coord);
}

void terminal_cursor_left(void) {
    printf("\b");
}

void terminal_cursor_right(void) {
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    
    COORD coord = csbi.dwCursorPosition;
    coord.X++;
    SetConsoleCursorPosition(hStdOut, coord);
}

key_event_t parse_key_sequence(void) {
    key_event_t event = {0};
    int c = _getch();

    if (c == '\r' || c == '\n') {
        event.type = KEY_ENTER;
        return event;
    }

    if (c == '\b' || c == 127) {
        event.type = KEY_BACKSPACE;
        return event;
    }

    if (c == 224) {  // Extended key prefix
        c = _getch();
        switch (c) {
            case 72: event.type = KEY_UP; break;
            case 80: event.type = KEY_DOWN; break;
            case 75: event.type = KEY_LEFT; break;
            case 77: event.type = KEY_RIGHT; break;
            case 71: event.type = KEY_HOME; break;
            case 79: event.type = KEY_END; break;
            case 83: event.type = KEY_DELETE; break;
            default:
                event.type = KEY_NORMAL;
                event.character = c;
                break;
        }
        return event;
    }

    // Normal character
    event.type = KEY_NORMAL;
    event.character = c;
    return event;
}

#else  // Unix/Linux

void terminal_raw_mode_enter(void) {
    if (raw_mode_active) return;

    // Save original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);

    struct termios raw = original_termios;

    // Disable line buffering and echo
    raw.c_lflag &= ~(ECHO | ICANON);

    // Set minimum characters and timeout for read
    raw.c_cc[VMIN] = 1;   // Read at least 1 character
    raw.c_cc[VTIME] = 0;  // No timeout

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_active = true;
}

void terminal_raw_mode_exit(void) {
    if (!raw_mode_active) return;

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    raw_mode_active = false;
}

void terminal_clear_eol(void) { 
    printf("\033[K"); 
    fflush(stdout);
}

void terminal_cursor_left(void) { 
    printf("\033[D"); 
    fflush(stdout);
}

void terminal_cursor_right(void) { 
    printf("\033[C"); 
    fflush(stdout);
}

key_event_t parse_key_sequence(void) {
    key_event_t event = {0};
    int c = getchar();

    if (c == EOF) {
        event.type = KEY_ENTER;
        return event;
    }

    if (c == '\r' || c == '\n') {
        event.type = KEY_ENTER;
        return event;
    }

    if (c == '\b' || c == 127) {
        event.type = KEY_BACKSPACE;
        return event;
    }


    if (c == '\033') {  // ESC
        // Read next character
        c = getchar();
        
        // Alt+key combinations (ESC followed by key)
        if (c == '\b' || c == 127) {
            event.type = KEY_ALT_BACKSPACE;
            return event;
        }
        
        
        if (c == '[') {
            // Read the command character
            c = getchar();
            switch (c) {
                case 'A': event.type = KEY_UP; break;
                case 'B': event.type = KEY_DOWN; break;
                case 'D': event.type = KEY_LEFT; break;
                case 'C': event.type = KEY_RIGHT; break;
                case 'H': event.type = KEY_HOME; break;
                case 'F': event.type = KEY_END; break;
                case '1':
                    // Handle sequences like ESC[1~ (Home key variant) or ESC[1;5D (Ctrl+Left) etc
                    c = getchar();
                    if (c == '~') {
                        event.type = KEY_HOME;
                    } else if (c == ';') {
                        // Ctrl+key sequences: ESC[1;5D, ESC[1;5C etc
                        c = getchar();
                        if (c == '5') {  // Ctrl modifier
                            c = getchar();
                            switch (c) {
                                case 'D': event.type = KEY_CTRL_LEFT; break;
                                case 'C': event.type = KEY_CTRL_RIGHT; break;
                                default:
                                    event.type = KEY_NORMAL;
                                    event.character = c;
                                    break;
                            }
                        } else {
                            event.type = KEY_NORMAL;
                            event.character = c;
                        }
                    } else {
                        event.type = KEY_NORMAL;
                        event.character = c;
                    }
                    break;
                case '3':
                    // Handle sequences like ESC[3~ (Delete key) or ESC[3;5~ (Ctrl+Delete)
                    c = getchar();
                    if (c == '~') {
                        event.type = KEY_DELETE;
                    } else if (c == ';') {
                        // Ctrl+Delete: ESC[3;5~
                        c = getchar();
                        if (c == '5') {  // Ctrl modifier
                            c = getchar();
                            if (c == '~') {
                                event.type = KEY_CTRL_DELETE;
                            } else {
                                event.type = KEY_NORMAL;
                                event.character = c;
                            }
                        } else {
                            event.type = KEY_NORMAL;
                            event.character = c;
                        }
                    } else {
                        event.type = KEY_NORMAL;
                        event.character = c;
                    }
                    break;
                case '4':
                    // Handle sequences like ESC[4~ (End key variant)
                    c = getchar();
                    if (c == '~') {
                        event.type = KEY_END;
                    } else {
                        event.type = KEY_NORMAL;
                        event.character = c;
                    }
                    break;
                default:
                    event.type = KEY_NORMAL;
                    event.character = c;
                    break;
            }
        } else {
            event.type = KEY_NORMAL;
            event.character = c;
        }
        return event;
    }

    // Normal character
    event.type = KEY_NORMAL;
    event.character = c;
    return event;
}

#endif

// Word boundary detection helpers
static int is_word_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || c == '_';
}

static size_t find_word_start_left(line_buffer_t *line) {
    if (line->cursor_pos == 0) return 0;
    
    size_t pos = line->cursor_pos;
    
    // If we're currently on a word character, move to start of current word
    if (pos > 0 && is_word_char(line->buffer[pos - 1])) {
        // Skip the word characters to find start of word
        while (pos > 0 && is_word_char(line->buffer[pos - 1])) {
            pos--;
        }
    } else {
        // Skip any non-word characters to the left
        while (pos > 0 && !is_word_char(line->buffer[pos - 1])) {
            pos--;
        }
        // Then skip the word characters to find start of word
        while (pos > 0 && is_word_char(line->buffer[pos - 1])) {
            pos--;
        }
    }
    
    return pos;
}

static size_t find_word_start_right(line_buffer_t *line) {
    if (line->cursor_pos >= line->length) return line->length;
    
    size_t pos = line->cursor_pos;
    
    // If we're currently on a word character, skip to end of current word first
    if (pos < line->length && is_word_char(line->buffer[pos])) {
        while (pos < line->length && is_word_char(line->buffer[pos])) {
            pos++;
        }
    }
    
    // Now skip any non-word characters to the right
    while (pos < line->length && !is_word_char(line->buffer[pos])) {
        pos++;
    }
    
    // We're now at the start of the next word (first character)
    return pos;
}

// Line buffer management functions
static void insert_char_at_cursor(line_buffer_t *line, char c) {
    if (line->length >= INPUT_BUFFER_SIZE - 1) {
        return;  // Buffer full
    }

    // Shift characters right from cursor position
    for (size_t i = line->length; i > line->cursor_pos; i--) {
        line->buffer[i] = line->buffer[i - 1];
    }

    // Insert new character
    line->buffer[line->cursor_pos] = c;
    line->length++;
    line->cursor_pos++;
}

static void delete_char_at_cursor(line_buffer_t *line) {
    if (line->cursor_pos == 0) {
        return;  // Nothing to delete
    }

    // Shift characters left from cursor position
    for (size_t i = line->cursor_pos - 1; i < line->length - 1; i++) {
        line->buffer[i] = line->buffer[i + 1];
    }

    line->length--;
    line->cursor_pos--;
}

static void clear_line_display(line_buffer_t *line) {
    // Move cursor to start of line
    while (line->cursor_pos > 0) {
        terminal_cursor_left();
        line->cursor_pos--;
    }

    // Clear to end of line
    terminal_clear_eol();
}

static void load_history_into_line(line_buffer_t *line, const char *hist_text) {
    // Clear current line display
    clear_line_display(line);

    // Load history text
    size_t hist_len = strlen(hist_text);
    if (hist_len >= INPUT_BUFFER_SIZE) {
        hist_len = INPUT_BUFFER_SIZE - 1;
    }

    memcpy(line->buffer, hist_text, hist_len);
    line->buffer[hist_len] = '\0';
    line->length = hist_len;
    line->cursor_pos = hist_len;

    // Display the text
    printf("%s", line->buffer);
    fflush(stdout);
}

static void redraw_from_cursor(line_buffer_t *line) {
    // Clear from cursor to end of line
    terminal_clear_eol();

    // Print remaining characters from cursor position
    printf("%.*s", (int)(line->length - line->cursor_pos), line->buffer + line->cursor_pos);

    // Move cursor back to original position
    size_t chars_printed = line->length - line->cursor_pos;
    for (size_t i = 0; i < chars_printed; i++) {
        terminal_cursor_left();
    }

    fflush(stdout);
}

static void move_cursor_left(line_buffer_t *line) {
    if (line->cursor_pos > 0) {
        line->cursor_pos--;
        terminal_cursor_left();
    }
}

static void move_cursor_right(line_buffer_t *line) {
    if (line->cursor_pos < line->length) {
        line->cursor_pos++;
        terminal_cursor_right();
    }
}

static void move_cursor_to_start(line_buffer_t *line) {
    while (line->cursor_pos > 0) {
        line->cursor_pos--;
        terminal_cursor_left();
    }
}

static void move_cursor_to_end(line_buffer_t *line) {
    while (line->cursor_pos < line->length) {
        line->cursor_pos++;
        terminal_cursor_right();
    }
}

static void move_cursor_word_left(line_buffer_t *line) {
    size_t target_pos = find_word_start_left(line);
    while (line->cursor_pos > target_pos) {
        line->cursor_pos--;
        terminal_cursor_left();
    }
}

static void move_cursor_word_right(line_buffer_t *line) {
    size_t target_pos = find_word_start_right(line);
    while (line->cursor_pos < target_pos) {
        line->cursor_pos++;
        terminal_cursor_right();
    }
}

static void handle_backspace(line_buffer_t *line) {
    if (line->cursor_pos > 0) {
        delete_char_at_cursor(line);
        // Move cursor back and redraw from new position
        terminal_cursor_left();
        redraw_from_cursor(line);
    }
}

static void delete_char_forward(line_buffer_t *line) {
    if (line->cursor_pos >= line->length) {
        return;  // Nothing to delete (cursor at end)
    }

    // Shift characters left from cursor position
    for (size_t i = line->cursor_pos; i < line->length - 1; i++) {
        line->buffer[i] = line->buffer[i + 1];
    }

    line->length--;
    // Note: cursor_pos stays the same for forward delete
}

static void handle_delete(line_buffer_t *line) {
    if (line->cursor_pos < line->length) {
        delete_char_forward(line);
        // Redraw from current position (cursor doesn't move)
        redraw_from_cursor(line);
    }
}

static void handle_alt_backspace(line_buffer_t *line) {
    // Delete word to the left of cursor
    size_t word_start = find_word_start_left(line);
    
    if (word_start < line->cursor_pos) {
        // Calculate how many characters to delete
        size_t chars_to_delete = line->cursor_pos - word_start;
        
        // Move data left to delete the word
        for (size_t i = word_start; i < line->length - chars_to_delete; i++) {
            line->buffer[i] = line->buffer[i + chars_to_delete];
        }
        
        line->length -= chars_to_delete;
        
        // Move cursor back to word start
        size_t old_cursor = line->cursor_pos;
        line->cursor_pos = word_start;
        for (size_t i = word_start; i < old_cursor; i++) {
            terminal_cursor_left();
        }
        
        // Redraw from word start position
        redraw_from_cursor(line);
    }
}

static void handle_ctrl_delete(line_buffer_t *line) {
    // Delete word to the right of cursor
    size_t word_end = find_word_start_right(line);
    
    if (word_end > line->cursor_pos) {
        // Calculate how many characters to delete
        size_t chars_to_delete = word_end - line->cursor_pos;
        
        // Move data left to delete the word
        for (size_t i = line->cursor_pos; i < line->length - chars_to_delete; i++) {
            line->buffer[i] = line->buffer[i + chars_to_delete];
        }
        
        line->length -= chars_to_delete;
        
        // Redraw from current cursor position
        redraw_from_cursor(line);
    }
}

static void handle_key_event(line_buffer_t *line, key_event_t event) {
    switch (event.type) {
        case KEY_NORMAL:
            if (event.character >= 32) {
                // When user starts typing, reset history view
                history_reset_view(&command_history);

                insert_char_at_cursor(line, event.character);
                // Print the character and redraw rest of line
                putchar(event.character);
                redraw_from_cursor(line);
            }
            break;

        case KEY_LEFT:
            move_cursor_left(line);
            break;

        case KEY_RIGHT:
            move_cursor_right(line);
            break;

        case KEY_UP: {
            const char *hist_text = history_get_previous(&command_history);
            if (hist_text) {
                load_history_into_line(line, hist_text);
            }
            break;
        }

        case KEY_DOWN: {
            const char *hist_text = history_get_next(&command_history);
            if (hist_text) {
                load_history_into_line(line, hist_text);
            } else {
                // Back to current (empty) line
                clear_line_display(line);
                line->length = 0;
                line->cursor_pos = 0;
            }
            break;
        }

        case KEY_HOME:
            move_cursor_to_start(line);
            break;

        case KEY_END:
            move_cursor_to_end(line);
            break;

        case KEY_BACKSPACE:
            // When user edits, reset history view
            history_reset_view(&command_history);
            handle_backspace(line);
            break;

        case KEY_DELETE:
            // When user edits, reset history view
            history_reset_view(&command_history);
            handle_delete(line);
            break;

        case KEY_CTRL_LEFT:
            move_cursor_word_left(line);
            break;

        case KEY_CTRL_RIGHT:
            move_cursor_word_right(line);
            break;

        case KEY_ALT_BACKSPACE:
            // When user edits, reset history view
            history_reset_view(&command_history);
            handle_alt_backspace(line);
            break;

        case KEY_CTRL_DELETE:
            // When user edits, reset history view
            history_reset_view(&command_history);
            handle_ctrl_delete(line);
            break;

        case KEY_ENTER:
            // Line complete - handled by caller
            break;
    }
}

// Main line editing function
void get_line_with_editing(char *buffer, size_t max_len) {
    line_buffer_t line = {0};

    // Initialize history on first use
    static bool history_initialized = false;
    if (!history_initialized) {
        history_init(&command_history);
        history_initialized = true;
    }

    terminal_raw_mode_enter();

    while (1) {
        key_event_t event = parse_key_sequence();

        if (event.type == KEY_ENTER) {
            break;
        }

        handle_key_event(&line, event);
    }

    terminal_raw_mode_exit();

    // Copy result to output buffer
    size_t copy_len = (line.length < max_len - 1) ? line.length : max_len - 1;
    memcpy(buffer, line.buffer, copy_len);
    buffer[copy_len] = '\0';

    // Add to history if non-empty
    if (copy_len > 0) {
        history_add(&command_history, buffer);
    }

    // Reset history view to current line
    history_reset_view(&command_history);

    printf("\n");
    fflush(stdout);
}