#include "timezone.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Forward declarations for internal functions
#ifdef EMBEDDED_TIMEZONE
static bool is_dst_active(const timezone_t* tz, int64_t epoch_millis);
static int get_day_of_year_for_dst_transition(int year, uint8_t month, uint8_t week, uint8_t weekday);
#endif

// UTC timezone (always available)
#ifdef FULL_TIMEZONE
static const timezone_t utc_timezone = {"UTC", "UTC"};
#elif defined(EMBEDDED_TIMEZONE)
static const timezone_t utc_timezone = {"UTC", 0, 0, NULL, "UTC", NULL};
#else
static const timezone_t utc_timezone = {"UTC", 0};
#endif

// System timezone pointer (set during initialization)
static const timezone_t* system_timezone = NULL;

#ifdef EMBEDDED_TIMEZONE
// Canadian timezone data (embedded mode)
static const dst_rule_t canada_dst_2007 = {
    .start_month = 3,    // March
    .start_week = 2,     // Second Sunday
    .start_day = 0,      // Sunday
    .start_hour = 2,     // 2:00 AM
    .end_month = 11,     // November
    .end_week = 1,       // First Sunday
    .end_day = 0,        // Sunday
    .end_hour = 2,       // 2:00 AM
    .start_year = 2007   // Effective from 2007
};

// Canadian timezone definitions
static const timezone_t canadian_timezones[] = {
    {"UTC", 0, 0, NULL, "UTC", NULL},
    {"America/St_Johns", -210, -150, &canada_dst_2007, "NST", "NDT"},      // UTC-3:30/-2:30 Newfoundland
    {"America/Halifax", -240, -180, &canada_dst_2007, "AST", "ADT"},       // UTC-4/-3 Atlantic
    {"America/Toronto", -300, -240, &canada_dst_2007, "EST", "EDT"},       // UTC-5/-4 Eastern
    {"America/Winnipeg", -360, -300, &canada_dst_2007, "CST", "CDT"},      // UTC-6/-5 Central
    {"America/Regina", -360, -360, NULL, "CST", NULL},                     // UTC-6 Saskatchewan (no DST)
    {"America/Edmonton", -420, -360, &canada_dst_2007, "MST", "MDT"},      // UTC-7/-6 Mountain
    {"America/Vancouver", -480, -420, &canada_dst_2007, "PST", "PDT"},     // UTC-8/-7 Pacific
};

static const size_t canadian_timezone_count = sizeof(canadian_timezones) / sizeof(canadian_timezones[0]);

#endif // EMBEDDED_TIMEZONE

// Initialize timezone system
void init_timezone_system(void) {
#ifdef FULL_TIMEZONE
    // For full timezone support, system timezone is determined at runtime
    // This is a placeholder - real implementation would detect system timezone
    system_timezone = &utc_timezone;
#elif defined(EMBEDDED_TIMEZONE)
    // For embedded timezone, default to UTC (can be overridden)
    system_timezone = &utc_timezone;
#else
    // For minimal timezone, only UTC is available
    system_timezone = &utc_timezone;
#endif
}

// Get UTC timezone
const timezone_t* timezone_utc(void) {
    return &utc_timezone;
}

// Get system default timezone
const timezone_t* timezone_system(void) {
    return system_timezone ? system_timezone : &utc_timezone;
}

// Get timezone by ID
const timezone_t* timezone_of(const char* timezone_id) {
    if (!timezone_id) {
        return NULL;
    }

    // Check for UTC
    if (strcmp(timezone_id, "UTC") == 0) {
        return &utc_timezone;
    }

#ifdef FULL_TIMEZONE
    // For full timezone support, validate timezone with system
    // Try to set the timezone and see if it works
    char* old_tz = getenv("TZ");
    setenv("TZ", timezone_id, 1);
    tzset();
    
    // Test if timezone is valid by getting current time
    time_t test_time = time(NULL);
    struct tm test_tm;
    struct tm* result = localtime_r(&test_time, &test_tm);
    
    // Restore original timezone
    if (old_tz) {
        setenv("TZ", old_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    if (result == NULL) {
        return NULL; // Invalid timezone
    }
    
    // Create timezone object using static storage (like UTC)
    // In a real implementation, you'd want a cache/pool for these
    static timezone_t full_timezones[100]; // Simple static pool
    static int timezone_count = 0;
    
    if (timezone_count >= 100) {
        return NULL; // Pool exhausted
    }
    
    timezone_t* tz = &full_timezones[timezone_count++];
    
    // Copy timezone ID to static storage
    static char timezone_ids[100][64]; // Static storage for IDs
    strncpy(timezone_ids[timezone_count-1], timezone_id, 63);
    timezone_ids[timezone_count-1][63] = '\0';
    
    tz->id = timezone_ids[timezone_count-1];
    tz->system_tz_name = timezone_ids[timezone_count-1];
    
    return tz;
    
#elif defined(EMBEDDED_TIMEZONE)
    // Search embedded timezone list
    for (size_t i = 0; i < canadian_timezone_count; i++) {
        if (strcmp(canadian_timezones[i].id, timezone_id) == 0) {
            return &canadian_timezones[i];
        }
    }
    return NULL; // Timezone not found in embedded set
    
#else
    // Minimal timezone: only UTC is supported
    return NULL;
#endif
}

// Get timezone offset for a specific time
int16_t timezone_get_offset(const timezone_t* tz, int64_t epoch_millis) {
    if (!tz) {
        return 0; // Default to UTC offset
    }

#ifdef FULL_TIMEZONE
    // Use system timezone functions
    time_t epoch_seconds = epoch_millis / 1000;
    
    // Set timezone environment variable
    char* old_tz = getenv("TZ");
    setenv("TZ", tz->system_tz_name ? tz->system_tz_name : tz->id, 1);
    tzset();
    
    // Get local time for this timezone
    struct tm local_tm;
    localtime_r(&epoch_seconds, &local_tm);
    int16_t offset_minutes = local_tm.tm_gmtoff / 60;
    
    // Restore original timezone
    if (old_tz) {
        setenv("TZ", old_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    return offset_minutes;
    
#elif defined(EMBEDDED_TIMEZONE)
    // Check if DST is active
    if (tz->dst_rule && is_dst_active(tz, epoch_millis)) {
        return tz->dst_offset;
    } else {
        return tz->standard_offset;
    }
    
#else
    // Minimal timezone: always UTC
    return tz->fixed_offset;
#endif
}

// Check if DST is active at a specific time
bool timezone_is_dst(const timezone_t* tz, int64_t epoch_millis) {
    if (!tz) {
        return false;
    }

#ifdef FULL_TIMEZONE
    // Use system timezone functions to determine DST
    time_t epoch_seconds = epoch_millis / 1000;
    
    char* old_tz = getenv("TZ");
    setenv("TZ", tz->system_tz_name ? tz->system_tz_name : tz->id, 1);
    tzset();
    
    struct tm local_tm;
    localtime_r(&epoch_seconds, &local_tm);
    bool is_dst = local_tm.tm_isdst > 0;
    
    // Restore original timezone
    if (old_tz) {
        setenv("TZ", old_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    return is_dst;
    
#elif defined(EMBEDDED_TIMEZONE)
    // Use embedded DST calculation
    return tz->dst_rule && is_dst_active(tz, epoch_millis);
    
#else
    // Minimal timezone: no DST
    return false;
#endif
}

// Get display name for timezone
const char* timezone_get_display_name(const timezone_t* tz, bool dst) {
    if (!tz) {
        return "UTC";
    }

#ifdef FULL_TIMEZONE
    // For full timezone, return the timezone ID as display name
    return tz->id;
    
#elif defined(EMBEDDED_TIMEZONE)
    // Return appropriate standard or DST name
    if (dst && tz->dst_name) {
        return tz->dst_name;
    } else {
        return tz->standard_name;
    }
    
#else
    // Minimal timezone
    return tz->id;
#endif
}

// Get timezone ID
const char* timezone_get_id(const timezone_t* tz) {
    return tz ? tz->id : "UTC";
}

// Validate timezone ID
bool is_valid_timezone_id(const char* id) {
    if (!id) {
        return false;
    }
    
    // UTC is always valid
    if (strcmp(id, "UTC") == 0) {
        return true;
    }
    
#ifdef FULL_TIMEZONE
    // For full timezone, we could validate against system timezone database
    // For now, accept any reasonable looking timezone ID
    return strstr(id, "/") != NULL; // Basic validation: must contain '/'
    
#elif defined(EMBEDDED_TIMEZONE)
    // Check against embedded timezone list
    for (size_t i = 0; i < canadian_timezone_count; i++) {
        if (strcmp(canadian_timezones[i].id, id) == 0) {
            return true;
        }
    }
    return false;
    
#else
    // Minimal timezone: only UTC is valid
    return false;
#endif
}

#ifdef EMBEDDED_TIMEZONE
// Helper function to check if DST is active at a given time
static bool is_dst_active(const timezone_t* tz, int64_t epoch_millis) {
    if (!tz->dst_rule) {
        return false; // No DST rules
    }
    
    // Convert epoch milliseconds to local time components
    time_t epoch_seconds = epoch_millis / 1000;
    struct tm utc_tm;
    gmtime_r(&epoch_seconds, &utc_tm);
    
    // Adjust to local standard time for DST calculation
    time_t local_seconds = epoch_seconds + (tz->standard_offset * 60);
    struct tm local_tm;
    gmtime_r(&local_seconds, &local_tm);
    
    int year = local_tm.tm_year + 1900;
    int day_of_year = local_tm.tm_yday + 1;
    int hour = local_tm.tm_hour;
    
    // Check if the rule applies to this year
    if (year < tz->dst_rule->start_year) {
        return false;
    }
    
    // Calculate DST transition days for this year
    int dst_start_day = get_day_of_year_for_dst_transition(
        year, tz->dst_rule->start_month, tz->dst_rule->start_week, tz->dst_rule->start_day
    );
    int dst_end_day = get_day_of_year_for_dst_transition(
        year, tz->dst_rule->end_month, tz->dst_rule->end_week, tz->dst_rule->end_day
    );
    
    // Check if we're in the DST period
    if (dst_start_day <= dst_end_day) {
        // DST period doesn't cross year boundary (e.g., March to November)
        if (day_of_year > dst_start_day && day_of_year < dst_end_day) {
            return true;
        } else if (day_of_year == dst_start_day && hour >= tz->dst_rule->start_hour) {
            return true;
        } else if (day_of_year == dst_end_day && hour < tz->dst_rule->end_hour) {
            return true;
        }
    } else {
        // DST period crosses year boundary (rare, but handle it)
        if (day_of_year > dst_start_day || day_of_year < dst_end_day) {
            return true;
        } else if (day_of_year == dst_start_day && hour >= tz->dst_rule->start_hour) {
            return true;
        } else if (day_of_year == dst_end_day && hour < tz->dst_rule->end_hour) {
            return true;
        }
    }
    
    return false;
}

// Calculate day of year for DST transitions (e.g., "2nd Sunday in March")
static int get_day_of_year_for_dst_transition(int year, uint8_t month, uint8_t week, uint8_t weekday) {
    // Days in each month (non-leap year)
    static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check for leap year
    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    
    // Calculate day of year for start of the target month
    int day_of_year = 1;
    for (int m = 1; m < month; m++) {
        day_of_year += days_in_month[m - 1];
        if (m == 2 && is_leap) {
            day_of_year++; // Add leap day
        }
    }
    
    // Find first day of the target month and its weekday
    time_t first_of_month = 0;
    struct tm tm_temp = {0};
    tm_temp.tm_year = year - 1900;
    tm_temp.tm_mon = month - 1;
    tm_temp.tm_mday = 1;
    first_of_month = mktime(&tm_temp);
    
    struct tm* first_tm = gmtime(&first_of_month);
    int first_weekday = first_tm->tm_wday; // 0=Sunday, 1=Monday, etc.
    
    // Calculate the target day
    int target_day;
    if (week == 0) {
        // Last occurrence of weekday in month
        int days_in_this_month = days_in_month[month - 1];
        if (month == 2 && is_leap) {
            days_in_this_month++;
        }
        
        // Find last occurrence
        for (target_day = days_in_this_month; target_day >= 1; target_day--) {
            int this_weekday = (first_weekday + target_day - 1) % 7;
            if (this_weekday == weekday) {
                break;
            }
        }
    } else {
        // Nth occurrence of weekday in month
        int occurrence_count = 0;
        for (target_day = 1; target_day <= 31; target_day++) {
            int this_weekday = (first_weekday + target_day - 1) % 7;
            if (this_weekday == weekday) {
                occurrence_count++;
                if (occurrence_count == week) {
                    break;
                }
            }
        }
    }
    
    return day_of_year + target_day - 1;
}

#endif // EMBEDDED_TIMEZONE