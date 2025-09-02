#pragma once
#include <stdio.h>
#include <stdlib.h>

// Forward declarations to avoid circular dependencies  
typedef struct slate_vm vm_t;

// Global VM pointer for library assert access
extern vm_t* g_current_vm;

// Wrapper function to handle library assert failures - implemented in runtime_error.c
void slate_library_assert_failed(const char* condition, const char* file, int line);

// Custom assert handler that routes to Slate error system
#define SLATE_LIBRARY_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            slate_library_assert_failed((message), __FILE__, __LINE__); \
        } \
    } while (0)

// Undefine existing library assert macros to prevent redefinition warnings
#ifdef DS_ASSERT
#undef DS_ASSERT
#endif

#ifdef DA_ASSERT
#undef DA_ASSERT
#endif

#ifdef DO_ASSERT
#undef DO_ASSERT
#endif

#ifdef DI_ASSERT
#undef DI_ASSERT
#endif

#ifdef STBDS_ASSERT
#undef STBDS_ASSERT
#endif

// Override all dynamic library assert macros
#define DS_ASSERT(cond) SLATE_LIBRARY_ASSERT(cond, #cond)
#define DA_ASSERT(cond) SLATE_LIBRARY_ASSERT(cond, #cond)
#define DO_ASSERT(cond) SLATE_LIBRARY_ASSERT(cond, #cond)
#define DI_ASSERT(cond) SLATE_LIBRARY_ASSERT(cond, #cond)
#define STBDS_ASSERT(cond) SLATE_LIBRARY_ASSERT(cond, #cond)