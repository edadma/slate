#ifndef SLATE_TIMEZONE_H
#define SLATE_TIMEZONE_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// DST (Daylight Saving Time) rule structure for embedded timezones
typedef struct {
    uint8_t start_month;    // Month when DST starts (1-12)
    uint8_t start_week;     // Week of month (1=first, 2=second, etc., 0=last)
    uint8_t start_day;      // Day of week (0=Sunday, 1=Monday, ..., 6=Saturday)
    uint8_t start_hour;     // Hour when DST starts (0-23)
    uint8_t end_month;      // Month when DST ends (1-12)
    uint8_t end_week;       // Week of month (1=first, 2=second, etc., 0=last)
    uint8_t end_day;        // Day of week (0=Sunday, 1=Monday, ..., 6=Saturday)
    uint8_t end_hour;       // Hour when DST ends (0-23)
    uint16_t start_year;    // Year when this rule became effective
} dst_rule_t;

// Timezone structure with conditional compilation
#ifdef FULL_TIMEZONE
// Full system timezone: use system timezone database
struct timezone {
    const char* id;                  // IANA timezone ID: "America/Toronto"
    char* system_tz_name;           // System timezone name for setenv("TZ")
};

#elif defined(EMBEDDED_TIMEZONE)
// Embedded timezone: static timezone data
struct timezone {
    const char* id;                 // IANA timezone ID: "America/Toronto"
    int16_t standard_offset;        // Standard time offset from UTC in minutes
    int16_t dst_offset;            // DST offset from UTC in minutes (same as standard if no DST)
    const dst_rule_t* dst_rule;    // DST transition rules (NULL if no DST)
    const char* standard_name;      // Standard time name: "EST"
    const char* dst_name;          // DST name: "EDT" (NULL if no DST)
};

#else
// Minimal timezone: UTC only
struct timezone {
    const char* id;           // "UTC"
    int16_t fixed_offset;     // Always 0 for UTC
};
#endif

// Typedef for timezone_t (works with all conditional compilation paths)
typedef struct timezone timezone_t;

// Timezone function prototypes (implementation varies by build type)
const timezone_t* timezone_utc(void);
const timezone_t* timezone_system(void);
const timezone_t* timezone_of(const char* timezone_id);

// Timezone operation functions
int16_t timezone_get_offset(const timezone_t* tz, int64_t epoch_millis);
bool timezone_is_dst(const timezone_t* tz, int64_t epoch_millis);
const char* timezone_get_display_name(const timezone_t* tz, bool dst);
const char* timezone_get_id(const timezone_t* tz);

// Utility functions
bool is_valid_timezone_id(const char* id);

// Initialize timezone system (called during VM startup)
void init_timezone_system(void);

#endif // SLATE_TIMEZONE_H