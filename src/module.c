#include "module.h"
#include "value.h"
#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

// Callback to copy a global variable to module exports
void copy_global_to_exports(const char* key, void* data, size_t size, void* context) {
    module_t* module = (module_t*)context;
    if (!module || !key || !data || size != sizeof(value_t)) {
        return;
    }
    
    value_t* value = (value_t*)data;
    do_set(module->exports, key, value, sizeof(value_t));
}

// Module system initialization
void module_system_init(struct slate_vm* vm) {
    if (!vm) return;
    
    // Initialize module cache
    vm->module_cache = do_create(NULL);
    
    // Initialize builtin modules  
    vm->builtin_modules = do_create(NULL);
    
    // Note: module_search_paths is now initialized in vm_create()
    
    // Set current module to NULL (will be set during module loading)
    vm->current_module = NULL;
}

// Module system cleanup
void module_system_cleanup(struct slate_vm* vm) {
    if (!vm) return;
    
    // Clean up module cache
    if (vm->module_cache) {
        // TODO: Properly release all cached modules
        do_release(&vm->module_cache);
    }
    
    // Clean up builtin modules
    if (vm->builtin_modules) {
        // TODO: Properly release all builtin modules
        do_release(&vm->builtin_modules);
    }
    
    // Note: module_search_paths is now cleaned up in vm_destroy()
    
    vm->current_module = NULL;
}

// Check if a file exists
int module_file_exists(const char* path) {
    if (!path) return 0;
    
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// Resolve module path to filesystem path
char* module_resolve_path(const char* module_name, const char* current_dir) {
    if (!module_name) return NULL;
    
    // Convert dots to slashes: "examples.modules.math" -> "examples/modules/math"
    size_t name_len = strlen(module_name);
    char* fs_path = malloc(name_len + 7); // +6 for ".slate" + 1 for null
    if (!fs_path) return NULL;
    
    strcpy(fs_path, module_name);
    
    // Replace dots with slashes
    for (char* p = fs_path; *p; p++) {
        if (*p == '.') *p = '/';
    }
    
    // Add .slate extension
    strcat(fs_path, ".slate");
    
    // Try different search locations
    char* full_path = NULL;
    
    // 1. Try as relative path from current directory
    if (current_dir) {
        size_t full_len = strlen(current_dir) + 1 + strlen(fs_path) + 1;
        full_path = malloc(full_len);
        if (full_path) {
            snprintf(full_path, full_len, "%s/%s", current_dir, fs_path);
            if (module_file_exists(full_path)) {
                free(fs_path);
                return full_path;
            }
            free(full_path);
            full_path = NULL;
        }
    }
    
    // 2. Try as absolute path from current working directory
    if (module_file_exists(fs_path)) {
        return fs_path;
    }
    
    // 3. Try from examples directory (for our demo modules)
    size_t examples_len = strlen("examples/") + strlen(fs_path) + 1;
    full_path = malloc(examples_len);
    if (full_path) {
        snprintf(full_path, examples_len, "examples/%s", fs_path);
        if (module_file_exists(full_path)) {
            free(fs_path);
            return full_path;
        }
        free(full_path);
    }
    
    free(fs_path);
    return NULL;
}

// Create a new module
module_t* module_create(const char* name, const char* path) {
    module_t* module = malloc(sizeof(module_t));
    if (!module) return NULL;
    
    module->name = name ? ds_new(name) : ds_new("");
    module->path = path ? ds_new(path) : ds_new("");
    module->exports = do_create(NULL);
    module->globals = do_create(NULL);
    module->state = MODULE_UNLOADED;
    module->init_function = NULL;
    module->ref_count = 1;
    
    return module;
}

// Destroy a module
void module_destroy(module_t* module) {
    if (!module) return;
    
    ds_release(&module->name);
    ds_release(&module->path);
    do_release(&module->exports);
    do_release(&module->globals);
    
    // TODO: Release init_function if needed
    
    free(module);
}

// Retain a module (increase reference count)
module_t* module_retain(module_t* module) {
    if (!module) return NULL;
    module->ref_count++;
    return module;
}

// Release a module (decrease reference count)
void module_release(module_t* module) {
    if (!module) return;
    
    module->ref_count--;
    if (module->ref_count <= 0) {
        module_destroy(module);
    }
}

// Export a value from a module
void module_export(module_t* module, const char* name, value_t value) {
    if (!module || !name) return;
    
    do_set(module->exports, name, &value, sizeof(value_t));
}

// Get an exported value from a module
value_t module_get_export(module_t* module, const char* name) {
    if (!module || !name) return make_null();
    
    if (do_has(module->exports, name)) {
        value_t* stored = (value_t*)do_get(module->exports, name);
        if (stored) {
            return *stored;
        }
    }
    
    return make_undefined(); // Export not found
}

// Check if a module has an export
int module_has_export(module_t* module, const char* name) {
    if (!module || !name) return 0;
    
    return do_has(module->exports, name);
}

// Load module from file
module_t* module_load_from_file(struct slate_vm* vm, const char* file_path) {
    if (!vm || !file_path) return NULL;
    
    // Read the file
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file contents
    char* source = malloc(file_size + 1);
    if (!source) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(source, 1, file_size, file);
    source[bytes_read] = '\0';
    fclose(file);
    
    // Create module
    module_t* module = module_create(file_path, file_path);
    if (!module) {
        free(source);
        return NULL;
    }
    
    module->state = MODULE_LOADING;
    
    // Create a new VM instance for module execution to avoid state corruption
    vm_t* module_vm = vm_create();
    if (!module_vm) {
        free(source);
        return NULL;
    }
    
    // Initialize module system in the module VM to support nested imports
    module_system_init(module_vm);
    
    // Copy search paths from parent VM to module VM
    size_t search_path_count = da_length(vm->module_search_paths);
    for (size_t i = 0; i < search_path_count; i++) {
        ds_string* search_path = (ds_string*)da_get(vm->module_search_paths, i);
        if (search_path) {
            module_add_search_path(module_vm, *search_path);
        }
    }
    
    // Execute module in isolation
    vm_result result = vm_interpret(module_vm, source);
    
    // Copy module globals to module exports (for import system)
    if (result == VM_OK) {
        // Copy all globals from the module VM to the module's exports
        do_foreach_property(module_vm->globals, copy_global_to_exports, module);
    }
    
    // Clean up module VM
    module_system_cleanup(module_vm);
    vm_destroy(module_vm);
    
    free(source);
    
    if (result == VM_OK) {
        module->state = MODULE_LOADED;
    } else {
        module->state = MODULE_UNLOADED;
    }
    
    return module;
}

// Load module by name
module_t* module_load(struct slate_vm* vm, const char* module_path) {
    if (!vm || !module_path) return NULL;
    
    // Check if module is already cached
    if (do_has(vm->module_cache, module_path)) {
        module_t** cached = (module_t**)do_get(vm->module_cache, module_path);
        if (cached && *cached) {
            return module_retain(*cached); // Return cached module with increased ref count
        }
    }
    
    // Resolve file system path using VM's search paths
    char* file_path = module_resolve_path_with_search_paths(vm, module_path, ".");
    if (!file_path) {
        return NULL; // Module not found
    }
    
    // Load from file
    module_t* module = module_load_from_file(vm, file_path);
    free(file_path);
    
    // Cache the module if successfully loaded
    if (module) {
        do_set(vm->module_cache, module_path, &module, sizeof(module_t*));
    }
    
    return module;
}

// === MODULE SEARCH PATH MANAGEMENT ===

// Add a search path to the VM's module search paths
void module_add_search_path(struct slate_vm* vm, const char* search_path) {
    if (!vm || !search_path) return;
    
    // Create a dynamic string copy of the search path
    ds_string path_copy = ds_new(search_path);
    
    // Add to the dynamic array
    da_push(vm->module_search_paths, &path_copy);
}

// Clear all search paths
void module_clear_search_paths(struct slate_vm* vm) {
    if (!vm) return;
    
    // Clear the array - da_clear will call release callbacks automatically
    da_clear(vm->module_search_paths);
}

// Get all search paths (caller must free the returned array, not the strings)
const char** module_get_search_paths(struct slate_vm* vm, size_t* count) {
    if (!vm || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    size_t path_count = da_length(vm->module_search_paths);
    *count = path_count;
    
    if (path_count == 0) return NULL;
    
    // Allocate array of const char* pointers
    const char** paths = malloc(path_count * sizeof(const char*));
    if (!paths) return NULL;
    
    // Fill array with pointers to the actual string data
    for (size_t i = 0; i < path_count; i++) {
        ds_string* path = (ds_string*)da_get(vm->module_search_paths, i);
        paths[i] = path ? *path : "";
    }
    
    return paths;
}

// Enhanced module path resolution that uses search paths
char* module_resolve_path_with_search_paths(struct slate_vm* vm, const char* module_name, const char* current_dir) {
    if (!module_name) return NULL;
    
    // First try the original resolution (current directory, examples/, etc.)
    char* resolved = module_resolve_path(module_name, current_dir);
    if (resolved) return resolved;
    
    // If not found and we have a VM, try the search paths
    if (!vm) return NULL;
    
    // Convert module name to filesystem path
    size_t name_len = strlen(module_name);
    char* fs_path = malloc(name_len + 7); // +6 for ".slate" + 1 for null
    if (!fs_path) return NULL;
    
    strcpy(fs_path, module_name);
    
    // Replace dots with slashes
    for (char* p = fs_path; *p; p++) {
        if (*p == '.') *p = '/';
    }
    
    // Add .slate extension
    strcat(fs_path, ".slate");
    
    // Try each search path
    size_t search_path_count = da_length(vm->module_search_paths);
    for (size_t i = 0; i < search_path_count; i++) {
        ds_string* search_path = (ds_string*)da_get(vm->module_search_paths, i);
        if (!search_path) continue;
        
        // Build full path: search_path/module_path
        size_t full_len = strlen(*search_path) + 1 + strlen(fs_path) + 1;
        char* full_path = malloc(full_len);
        if (!full_path) continue;
        
        snprintf(full_path, full_len, "%s/%s", *search_path, fs_path);
        
        // Check if file exists
        if (module_file_exists(full_path)) {
            free(fs_path);
            return full_path;
        }
        
        free(full_path);
    }
    
    free(fs_path);
    return NULL;
}