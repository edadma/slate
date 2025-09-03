#ifndef SLATE_MODULE_H
#define SLATE_MODULE_H

#include "value.h"
#include "dynamic_object.h"
#include "dynamic_string.h"

// Forward declarations
typedef struct vm_t vm_t;
typedef struct function_t function_t;

// Module state
typedef enum {
    MODULE_UNLOADED,
    MODULE_LOADING,    // Prevents circular dependencies
    MODULE_LOADED
} module_state_t;

// Module structure
typedef struct module_t {
    ds_string name;           // Module name (e.g., "examples.modules.math")
    ds_string path;           // File system path
    do_object exports;        // Exported symbols
    do_object globals;        // Module's global namespace
    module_state_t state;     // Loading state
    function_t* init_function; // Module initialization code
    int ref_count;            // Reference counting
} module_t;

// Module cache entry
typedef struct module_cache_entry {
    ds_string key;            // Module path/name
    module_t* module;         // Cached module
} module_cache_entry;

// Module system functions
void module_system_init(vm_t* vm);
void module_system_cleanup(vm_t* vm);

// Module loading
module_t* module_load(vm_t* vm, const char* module_path);
module_t* module_load_from_file(vm_t* vm, const char* file_path);
module_t* module_load_from_directory(vm_t* vm, const char* dir_path);

// Module resolution
char* module_resolve_path(const char* module_name, const char* current_dir);
int module_file_exists(const char* path);

// Module management
module_t* module_create(const char* name, const char* path);
void module_destroy(module_t* module);
module_t* module_retain(module_t* module);
void module_release(module_t* module);

// Module exports
void module_export(module_t* module, const char* name, value_t value);
value_t module_get_export(module_t* module, const char* name);
int module_has_export(module_t* module, const char* name);

// Built-in modules
void module_register_builtin(vm_t* vm, const char* name, module_t* module);
module_t* module_create_math(vm_t* vm);
module_t* module_create_io(vm_t* vm);
module_t* module_create_sys(vm_t* vm);

#endif // SLATE_MODULE_H