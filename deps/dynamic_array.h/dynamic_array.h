/**
 * @file dynamic_array.h
 * @brief reference-counted (optionally atomic) dynamic arrays with ArrayBuffer-style builder
 * @author dynamic_array.h contributors
 * @version 0.1.1
 * @date August 2025
 *
 * Single header library for mutable dynamic arrays with reference counting.
 * Includes Scala ArrayBuffer-style builder for efficient construction.
 * Designed for portability across PC and microcontroller targets.
 *
 * @section config Configuration
 *
 * Customize the library by defining these macros before including:
 *
 * @code
 * #define DA_MALLOC malloc         // custom allocator
 * #define DA_REALLOC realloc       // custom reallocator
 * #define DA_FREE free             // custom deallocator
 * #define DA_ASSERT assert         // custom assert macro
 * #define DA_GROWTH 16             // fixed growth increment (default: doubling)
 * #define DA_ATOMIC_REFCOUNT 1     // enable atomic reference counting (C11 required)
 *
 * #define DA_IMPLEMENTATION
 * #include "dynamic_array.h"
 * @endcode
 *
 * @section usage Basic Usage
 *
 * @subsection arrays Regular Arrays
 * @code
 * da_array arr = da_new(sizeof(int), 10);
 * DA_PUSH(arr, 42);
 * int val = DA_AT(arr, 0, int);
 * da_release(&arr);  // arr becomes NULL
 * @endcode
 *
 * @subsection builders Builder Pattern (like Scala's ArrayBuffer)
 * @code
 * da_builder builder = DA_BUILDER_CREATE(int);
 * DA_BUILDER_APPEND(builder, 42);
 * DA_BUILDER_APPEND(builder, 99);
 * da_array arr = da_builder_to_array(&builder);  // Exact size, builder becomes NULL
 * da_release(&arr);
 * @endcode
 *
 * @section atomic Atomic Reference Counting
 *
 * When DA_ATOMIC_REFCOUNT=1 (requires C11):
 * - Reference counting operations (da_retain/da_release) are lock-free
 * - Array content modifications require external synchronization
 * - Builders are not thread-safe and should be used by single threads
 *
 * @section platforms Supported Platforms
 *
 * - Linux (GCC, Clang)
 * - Windows (MinGW, MSVC)
 * - Raspberry Pi Pico (arm-none-eabi-gcc)
 * - ESP32-C3 (Espressif toolchain)
 * - ARM Cortex-M (various toolchains)
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @defgroup config Configuration Macros
 * @brief Customizable macros for memory allocation and behavior
 * @{
 */

/** @brief Custom memory allocator (default: malloc) */
#ifndef DA_MALLOC
#define DA_MALLOC malloc
#endif

/** @brief Custom memory reallocator (default: realloc) */
#ifndef DA_REALLOC
#define DA_REALLOC realloc
#endif

/** @brief Custom memory deallocator (default: free) */
#ifndef DA_FREE
#define DA_FREE free
#endif

/** @brief Custom assertion macro (default: assert) */
#ifndef DA_ASSERT
#define DA_ASSERT assert
#endif

#ifndef DA_DEF
    #ifdef DA_STATIC
        #define DA_DEF static
    #else
        #define DA_DEF extern
    #endif
#endif

/**
 * @brief Enable atomic reference counting (default: 0)
 * @note Requires C11 and stdatomic.h support
 * @note Only reference counting is atomic/thread-safe, not array operations
 * @warning Array modifications (push/pop/set) still require external synchronization
 */
#ifndef DA_ATOMIC_REFCOUNT
#define DA_ATOMIC_REFCOUNT 0
#endif

/** @} */ // end of config group

/* Check C11 support for atomic operations */
#if DA_ATOMIC_REFCOUNT && __STDC_VERSION__ < 201112L
    #error "DA_ATOMIC_REFCOUNT requires C11 or later for atomic support (compile with -std=c11 or later)"
#endif

/* atomic operations */
#if DA_ATOMIC_REFCOUNT
    #include <stdatomic.h>
    #define DA_ATOMIC_INT _Atomic int
    #define DA_ATOMIC_FETCH_ADD(ptr, val) atomic_fetch_add(ptr, val)
    #define DA_ATOMIC_FETCH_SUB(ptr, val) atomic_fetch_sub(ptr, val)
    #define DA_ATOMIC_LOAD(ptr) atomic_load(ptr)
    #define DA_ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
#else
    #define DA_ATOMIC_INT int
    #define DA_ATOMIC_FETCH_ADD(ptr, val) (*(ptr) += (val), *(ptr) - (val))
    #define DA_ATOMIC_FETCH_SUB(ptr, val) (*(ptr) -= (val), *(ptr) + (val))
    #define DA_ATOMIC_LOAD(ptr) (*(ptr))
    #define DA_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
#endif

/* Detect C23/C++11 auto support (preferred) or typeof fallback */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L && !defined(__STDC_NO_TYPEOF__)
    #define DA_TYPEOF(x) typeof(x)     /* the C23 typeof keyword */
    #define DA_HAS_TYPEOF 1
#elif defined(__cplusplus) && __cplusplus >= 201103L
    #define DA_TYPEOF(x) decltype(x)   /* the C++ decltype keyword */
    #define DA_HAS_TYPEOF 1
#elif (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
    #define DA_TYPEOF(x) typeof(x)
    #define DA_HAS_TYPEOF 1
#else
    #define DA_HAS_TYPEOF 0
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define DA_AUTO auto   /* the C23 auto keyword */
    #define DA_HAS_AUTO 1
#elif defined(__cplusplus) && __cplusplus >= 201103L
    #define DA_AUTO auto   /* the C++ auto keyword */
    #define DA_HAS_AUTO 1
#elif (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
    #define DA_AUTO __auto_type
    #define DA_HAS_AUTO 1
#else
    #define DA_HAS_AUTO 0
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define DA_GENERIC(x, ...) _Generic((x), __VA_ARGS__)
    #define DA_HAS_GENERIC 1
#else
    #define DA_HAS_GENERIC 0
#endif

#if DA_HAS_AUTO
    #define DA_SUPPORT_TYPE_INFERENCE 1
    #define DA_MAKE_VAR_WITH_INFERRED_TYPE(name, initializer) DA_AUTO (name) = (initializer);
#elif DA_HAS_TYPEOF
    #define DA_SUPPORT_TYPE_INFERENCE 1
    #define DA_MAKE_VAR_WITH_INFERRED_TYPE(name, initializer) DA_TYPEOF(initializer) (name) = (initializer);
#else
    #define DA_SUPPORT_TYPE_INFERENCE 0
#endif

/**
 * @defgroup types Core Types
 * @brief Main data structures for arrays and builders
 * @{
 */

/**
 * @brief Reference-counted dynamic array structure
 * @note Do not access fields directly - use provided functions and macros
 * @note Atomic reference counting when DA_ATOMIC_REFCOUNT=1
 */
typedef struct {
    DA_ATOMIC_INT ref_count;  /**< @brief Reference count (atomic if DA_ATOMIC_REFCOUNT=1) */
    int length;               /**< @brief Current number of elements */
    int capacity;             /**< @brief Allocated capacity */
    int element_size;         /**< @brief Size of each element in bytes */
    void *data;               /**< @brief Pointer to element data */
    void (*retain_fn)(void*); /**< @brief Optional retain function called when elements added (NULL if not needed) */
    void (*release_fn)(void*); /**< @brief Optional release function called when elements removed (NULL if not needed) */
} da_array_t, *da_array;

/**
 * @brief ArrayBuffer-style builder for efficient array construction
 * @note Not thread-safe
 * @note Always uses doubling growth strategy for fast construction
 * @note Convert to da_array with da_builder_to_array() for sharing/efficiency
 */
typedef struct {
    int length;               /**< @brief Current number of elements */
    int capacity;             /**< @brief Allocated capacity */
    int element_size;         /**< @brief Size of each element in bytes */
    void *data;               /**< @brief Pointer to element data */
} da_builder_t, *da_builder;

/** @} */ // end of types group

/**
 * @defgroup array_lifecycle Array Lifecycle
 * @brief Functions for creating, sharing, and destroying arrays
 * @{
 */

/**
 * @brief Creates a new dynamic array (simple version for general use)
 * @param element_size Size in bytes of each element (must be > 0)
 * @return New array with ref_count = 1, capacity = 0 (deferred allocation)
 * @note Asserts on allocation failure or invalid parameters
 * @note Uses configured growth strategy (DA_GROWTH) for expansions
 * @note Atomic reference counting if DA_ATOMIC_REFCOUNT=1
 * @note No retain/release functions - suitable for simple types
 *
 * @code
 * da_array arr = da_new(sizeof(int));
 * DA_PUSH(arr, 42);
 * da_release(&arr);
 * @endcode
 */
DA_DEF da_array da_new(int element_size);

/**
 * @brief Creates a new dynamic array with full configuration
 * @param element_size Size in bytes of each element (must be > 0)
 * @param initial_capacity Initial capacity (0 is valid for deferred allocation)
 * @param retain_fn Optional retain function called when elements are added (NULL if not needed)
 * @param release_fn Optional release function called when elements are removed (NULL if not needed)
 * @return New array with ref_count = 1
 * @note Asserts on allocation failure or invalid parameters
 * @note Uses configured growth strategy (DA_GROWTH) for expansions
 * @note Atomic reference counting if DA_ATOMIC_REFCOUNT=1
 * @note retain_fn is called during da_push(), da_insert(), da_set(), etc.
 * @note release_fn is called during da_release(), da_pop(), da_remove(), da_clear(), etc.
 *
 * @code
 * // Reference-counted types (like Metal's cell_t)
 * da_array cells = da_create(sizeof(cell_t), 100, 
 *                           (void(*)(void*))retain,   // retain on add
 *                           (void(*)(void*))release); // release on remove
 * @endcode
 */
DA_DEF da_array da_create(int element_size, int initial_capacity, void (*retain_fn)(void*), void (*release_fn)(void*));

/**
 * @brief Releases a reference to an array, potentially freeing it
 * @param arr Pointer to array pointer (will be set to NULL)
 * @note Always sets *arr to NULL for safety, regardless of ref count
 * @note Only frees memory when ref_count reaches 0
 * @note Thread-safe if DA_ATOMIC_REFCOUNT=1
 * @note Asserts if arr or *arr is NULL
 *
 * @code
 * da_array arr = da_new(sizeof(int), 5, NULL);
 * da_release(&arr);  // arr becomes NULL, memory freed
 * @endcode
 */
DA_DEF void da_release(da_array* arr);

/**
 * @brief Increments reference count for sharing an array
 * @param arr Array to retain (must not be NULL)
 * @return The same array pointer (for convenience)
 * @note Thread-safe if DA_ATOMIC_REFCOUNT=1
 * @note Use da_release() to decrement reference count
 *
 * @code
 * da_array shared = da_retain(original_array);
 * pass_to_worker_thread(shared);
 * da_release(&shared);  // Decrements count, shared becomes NULL
 * @endcode
 */
DA_DEF da_array da_retain(da_array arr);

/** @} */ // end of array_lifecycle group

/**
 * @defgroup array_access Array Access
 * @brief Functions for reading and writing array elements
 * @{
 */

/**
 * @brief Gets a pointer to an element at the specified index
 * @param arr Array to access (must not be NULL)
 * @param index Element index (must be >= 0 and < length)
 * @return Pointer to element at index
 * @note Asserts on out-of-bounds access
 * @note Returned pointer is valid until array is modified or released
 *
 * @code
 * int* ptr = (int*)da_get(arr, 0);
 * *ptr = 42;  // Direct modification
 * @endcode
 */
DA_DEF void* da_get(da_array arr, int index);

/**
 * @brief Gets direct pointer to the underlying data array
 * @param arr Array to access (must not be NULL)
 * @return Pointer to raw data array (like stb_ds.h style)
 * @note Enables direct indexing: ((int*)da_data(arr))[i]
 * @note Pointer is valid until array is modified or released
 * @note No bounds checking - use with care
 *
 * @code
 * int* data = (int*)da_data(arr);
 * data[0] = 42;  // Direct array-style access
 * data[1] = 99;
 * @endcode
 */
DA_DEF void* da_data(da_array arr);

/**
 * @brief Sets the value of an element at the specified index
 * @param arr Array to modify (must not be NULL)
 * @param index Element index (must be >= 0 and < length)
 * @param element Pointer to element data to copy (must not be NULL)
 * @note Asserts on out-of-bounds access or NULL parameters
 * @note Copies element_size bytes from element pointer
 *
 * @code
 * int value = 42;
 * da_set(arr, 0, &value);
 * @endcode
 */
DA_DEF void da_set(da_array arr, int index, const void* element);

/** @} */ // end of array_access group

/**
 * @defgroup array_modification Array Modification
 * @brief Functions for adding, removing, and clearing elements
 * @{
 */

/**
 * @brief Appends an element to the end of the array
 * @param arr Array to modify (must not be NULL)
 * @param element Pointer to element data to copy (must not be NULL)
 * @note Automatically grows array capacity if needed
 * @note Asserts on allocation failure or NULL parameters
 * @note Uses configured growth strategy (DA_GROWTH)
 *
 * @code
 * int value = 42;
 * da_push(arr, &value);
 * @endcode
 */
DA_DEF void da_push(da_array arr, const void* element);

/**
 * @brief Inserts an element at the specified index
 * @param arr Array to modify (must not be NULL)
 * @param index Position to insert at (must be >= 0 and <= length)
 * @param element Pointer to element data to copy (must not be NULL)
 * @note Shifts all elements at and after index to the right
 * @note Automatically grows array capacity if needed
 * @note index == length is equivalent to da_push()
 * @note Asserts on out-of-bounds index or NULL parameters
 *
 * @code
 * int value = 42;
 * da_insert(arr, 0, &value);  // Insert at beginning
 * da_insert(arr, da_length(arr), &value);  // Insert at end (same as push)
 * @endcode
 */
DA_DEF void da_insert(da_array arr, int index, const void* element);

/**
 * @brief Removes and optionally returns an element at the specified index
 * @param arr Array to modify (must not be NULL)
 * @param index Position to remove from (must be >= 0 and < length)
 * @param out Optional pointer to store removed element (can be NULL)
 * @note Shifts all elements after index to the left
 * @note Does not shrink capacity
 * @note Asserts on out-of-bounds index
 *
 * @code
 * int removed;
 * da_remove(arr, 0, &removed);  // Remove first element
 * da_remove(arr, 2, NULL);      // Remove third element, discard value
 * @endcode
 */
DA_DEF void da_remove(da_array arr, int index, void* out);

/**
 * @brief Removes and optionally returns the last element
 * @param arr Array to modify (must not be NULL)
 * @param out Optional pointer to store popped element (can be NULL)
 * @note Asserts if array is empty
 * @note If out is NULL, element is discarded
 * @note Does not shrink capacity
 *
 * @code
 * int popped;
 * da_pop(arr, &popped);  // Get the value
 * da_pop(arr, NULL);     // Discard the value
 * @endcode
 */
DA_DEF void da_pop(da_array arr, void* out);

/**
 * @brief Removes all elements from the array
 * @param arr Array to clear (must not be NULL)
 * @note Sets length to 0 but preserves capacity
 * @note Does not free allocated memory (use da_resize(arr, 0) for that)
 *
 * @code
 * da_clear(arr);
 * assert(da_length(arr) == 0);
 * @endcode
 */
DA_DEF void da_clear(da_array arr);

/**
 * @brief Gets a pointer to the last element without removing it
 * @param arr Array to peek (must not be NULL and not empty)
 * @return Pointer to the last element
 * @note Asserts if array is empty
 * @note Returned pointer is valid until array is modified or released
 * @note Useful for stack-like access patterns
 *
 * @code
 * int* last_ptr = (int*)da_peek(arr);
 * printf("Last element: %d\n", *last_ptr);
 * @endcode
 */
DA_DEF void* da_peek(da_array arr);

/**
 * @brief Gets a pointer to the first element without removing it
 * @param arr Array to peek (must not be NULL and not empty)
 * @return Pointer to the first element
 * @note Asserts if array is empty
 * @note Returned pointer is valid until array is modified or released
 * @note Useful for queue-like access patterns
 *
 * @code
 * int* first_ptr = (int*)da_peek_first(arr);
 * printf("First element: %d\n", *first_ptr);
 * @endcode
 */
DA_DEF void* da_peek_first(da_array arr);

/** @} */ // end of array_modification group

/**
 * @defgroup array_utility Array Utility
 * @brief Functions for querying and managing array size and capacity
 * @{
 */

/**
 * @brief Gets the current number of elements in the array
 * @param arr Array to query (must not be NULL)
 * @return Number of elements currently in the array
 *
 * @code
 * for (int i = 0; i < da_length(arr); i++) {
 *     process_element(da_get(arr, i));
 * }
 * @endcode
 */
DA_DEF int da_length(da_array arr);

/**
 * @brief Gets the current allocated capacity of the array
 * @param arr Array to query (must not be NULL)
 * @return Number of elements that can be stored without reallocation
 *
 * @code
 * printf("Array using %d/%d slots\n", da_length(arr), da_capacity(arr));
 * @endcode
 */
DA_DEF int da_capacity(da_array arr);

/**
 * @brief Ensures the array has at least the specified capacity
 * @param arr Array to modify (must not be NULL)
 * @param new_capacity Minimum capacity required (must be >= 0)
 * @note Only increases capacity, never decreases
 * @note Asserts on allocation failure
 * @note Useful for avoiding multiple reallocations when size is known
 *
 * @code
 * da_reserve(arr, 1000);  // Ensure space for 1000 elements
 * for (int i = 0; i < 1000; i++) {
 *     da_push(arr, &i);  // No reallocations needed
 * }
 * @endcode
 */
DA_DEF void da_reserve(da_array arr, int new_capacity);

/**
 * @brief Changes the array length, growing or shrinking as needed
 * @param arr Array to modify (must not be NULL)
 * @param new_length New length for the array (must be >= 0)
 * @note If growing, new elements are zero-initialized
 * @note If shrinking, excess elements are discarded
 * @note Automatically adjusts capacity if needed
 * @note Asserts on allocation failure
 *
 * @code
 * da_resize(arr, 100);  // Array now has exactly 100 elements
 * // Elements 0..old_length-1 preserve their values
 * // Elements old_length..99 are zero-initialized
 * @endcode
 */
DA_DEF void da_resize(da_array arr, int new_length);

/**
 * @brief Reduces the array's allocated capacity to a specific size
 * @param arr Array to modify (must not be NULL)
 * @param new_capacity New capacity for the array (must be >= length)
 * @note Only reduces capacity, never increases
 * @note Useful for memory optimization after removing many elements
 * @note Asserts if new_capacity < current length
 * @note Asserts on allocation failure
 *
 * @code
 * da_array arr = DA_CREATE(int, 1000);  // capacity = 1000
 * // ... add 50 elements, remove 30, now length = 20
 * da_trim(arr, 30);  // capacity = 30, saves memory
 * @endcode
 */
DA_DEF void da_trim(da_array arr, int new_capacity);

/**
 * @brief Appends all elements from source array to destination array
 * @param dest Destination array to append to (must not be NULL)
 * @param src Source array to read from (must not be NULL)
 * @note Automatically grows dest capacity if needed
 * @note Source array is unchanged
 * @note Arrays must have the same element_size
 * @note Asserts on allocation failure or mismatched element sizes
 *
 * @code
 * da_array arr1 = DA_CREATE(int, 2);  // [10, 20]
 * da_array arr2 = DA_CREATE(int, 2);  // [30, 40]
 * da_append_array(arr1, arr2);        // arr1 = [10, 20, 30, 40]
 * @endcode
 */
DA_DEF void da_append_array(da_array dest, da_array src);

/**
 * @brief Creates a new array by concatenating two arrays
 * @param arr1 First array (must not be NULL)
 * @param arr2 Second array (must not be NULL)
 * @return New array containing elements from arr1 followed by arr2
 * @note Arrays must have the same element_size
 * @note Original arrays are unchanged
 * @note Returned array has ref_count = 1
 * @note Capacity is exactly length (no wasted space)
 * @note Asserts on allocation failure or mismatched element sizes
 *
 * @code
 * da_array nums1 = DA_CREATE(int, 2);  // [10, 20]
 * da_array nums2 = DA_CREATE(int, 2);  // [30, 40]
 * da_array combined = da_concat(nums1, nums2);  // [10, 20, 30, 40]
 * // nums1 and nums2 remain unchanged
 * @endcode
 */
DA_DEF da_array da_concat(da_array arr1, da_array arr2);

/**
 * @brief Appends raw C array data to the dynamic array
 * @param arr Array to modify (must not be NULL)
 * @param data Pointer to raw data array (must not be NULL)
 * @param count Number of elements to append (must be >= 0)
 * @note Automatically grows array capacity if needed
 * @note More efficient than multiple da_push() calls
 * @note Asserts on allocation failure or NULL parameters
 *
 * @code
 * int raw_data[] = {10, 20, 30, 40};
 * da_append_raw(arr, raw_data, 4);  // Append all 4 elements at once
 * @endcode
 */
DA_DEF void da_append_raw(da_array arr, const void* data, int count);

/**
 * @brief Fills the array with multiple copies of an element
 * @param arr Array to modify (must not be NULL)
 * @param element Pointer to element to replicate (must not be NULL)
 * @param count Number of copies to add (must be >= 0)
 * @note Automatically grows array capacity if needed
 * @note More efficient than multiple da_push() calls
 * @note Asserts on allocation failure or NULL parameters
 *
 * @code
 * int zero = 0;
 * da_fill(arr, &zero, 100);  // Add 100 zeros to array
 * @endcode
 */
DA_DEF void da_fill(da_array arr, const void* element, int count);

/**
 * @brief Creates a new array containing elements from a range [start, end)
 * @param arr Source array to slice from (must not be NULL)
 * @param start Starting index (inclusive, must be >= 0)
 * @param end Ending index (exclusive, must be >= start and <= length)
 * @return New array containing elements from [start, end)
 * @note Original array is unchanged
 * @note Returned array has ref_count = 1 and exact capacity
 * @note Empty range (start == end) returns empty array
 * @note Asserts on out-of-bounds indices or allocation failure
 *
 * @code
 * da_array original = DA_CREATE(int, 5);  // [10, 20, 30, 40, 50]
 * da_array slice = da_slice(original, 1, 4);  // [20, 30, 40]
 * @endcode
 */
DA_DEF da_array da_slice(da_array arr, int start, int end);

/**
 * @brief Creates a complete copy of an existing array
 * @param arr Source array to copy from (must not be NULL)
 * @return New array containing all elements from the source array
 * @note Original array is unchanged
 * @note Returned array has ref_count = 1 and exact capacity = length
 * @note Perfect for creating arrays that can be modified independently
 * @note Asserts on allocation failure
 *
 * @code
 * da_array original = DA_CREATE(int, 3);  // [10, 20, 30]
 * da_array copy = da_copy(original);      // [10, 20, 30] - independent copy
 *
 * // Now you can sort the copy without affecting the original
 * sort_array(copy);  // original remains [10, 20, 30]
 * @endcode
 */
DA_DEF da_array da_copy(da_array arr);

/**
 * @brief Creates a new array containing elements that pass a predicate test
 * @param arr Source array to filter (must not be NULL)
 * @param predicate Function that returns non-zero for elements to keep (must not be NULL)
 * @param context Optional context pointer passed to predicate function (can be NULL)
 * @return New array containing only elements that pass the predicate test (exact capacity)
 * @note Creates a new array with reference count = 1
 * @note Result array has exact capacity = number of matching elements (no wasted memory)
 * @note Returns empty array (capacity=0) if no elements match
 * @note Predicate receives (element_pointer, context) and should return non-zero to keep element
 * @note Asserts on allocation failure or NULL required parameters
 * @note Perfect for functional programming patterns and data filtering
 *
 * @code
 * // Filter even numbers
 * int is_even(const void* elem, void* ctx) {
 *     return *(int*)elem % 2 == 0;
 * }
 *
 * da_array numbers = DA_CREATE(int, 5);  // [1, 2, 3, 4, 5]
 * da_array evens = da_filter(numbers, is_even, NULL);  // [2, 4] - exact capacity
 * @endcode
 */
DA_DEF da_array da_filter(da_array arr, int (*predicate)(const void* element, void* context), void* context);

/**
 * @brief Creates a new array by transforming each element using a mapper function
 * @param arr Source array to transform (must not be NULL)
 * @param mapper Function to transform elements (must not be NULL)
 * @param context Optional context pointer passed to mapper function (can be NULL)
 * @return New array with transformed elements (same length, exact capacity)
 * @note Creates a new array with reference count = 1
 * @note Result array has same length as source and exact capacity = length
 * @note Mapper receives (src_element_ptr, dst_element_ptr, context)
 * @note Mapper should write transformed result to dst_element_ptr
 * @note Asserts on allocation failure or NULL required parameters
 * @note Perfect for functional programming patterns and data transformation
 *
 * @code
 * // Double all values
 * void double_int(const void* src, void* dst, void* ctx) {
 *     *(int*)dst = *(int*)src * 2;
 * }
 *
 * da_array numbers = DA_CREATE(int, 3);    // [1, 2, 3]
 * da_array doubled = da_map(numbers, double_int, NULL);  // [2, 4, 6] - exact capacity
 * @endcode
 */
DA_DEF da_array da_map(da_array arr, void (*mapper)(const void* src, void* dst, void* context), void* context);

/**
 * @brief Reduces array to single value using accumulator function
 * @param arr Source array (must not be NULL)
 * @param initial Initial accumulator value (must not be NULL)
 * @param result Output buffer for final result (must not be NULL)
 * @param reducer Function that combines accumulator with each element
 * @param context Optional context passed to reducer function (can be NULL)
 * @note Reducer function signature: void (*reducer)(void* accumulator, const void* element, void* context)
 * @note The accumulator is passed as first parameter and modified in-place
 * @note Result buffer receives the final accumulated value
 *
 * @code
 * // Sum all integers
 * void sum_ints(void* acc, const void* elem, void* ctx) {
 *     (void)ctx;
 *     *(int*)acc += *(int*)elem;
 * }
 *
 * int initial = 0;
 * int result;
 * da_reduce(numbers, &initial, &result, sum_ints, NULL);  // result = sum of all elements
 * @endcode
 */
DA_DEF void da_reduce(da_array arr, const void* initial, void* result,
                      void (*reducer)(void* accumulator, const void* element, void* context), void* context);

/**
 * @brief Removes multiple consecutive elements from the array
 * @param arr Array to modify (must not be NULL)
 * @param start Starting index of range to remove (must be >= 0)
 * @param count Number of elements to remove (must be >= 0)
 * @note Shifts elements after the range to the left
 * @note Does not shrink capacity
 * @note Asserts on out-of-bounds range
 *
 * @code
 * // Remove elements at indices 2, 3, 4 (3 elements starting at index 2)
 * da_remove_range(arr, 2, 3);
 * @endcode
 */
DA_DEF void da_remove_range(da_array arr, int start, int count);

/**
 * @brief Reverses all elements in the array in place
 * @param arr Array to reverse (must not be NULL)
 * @note Modifies the array directly
 * @note O(n/2) time complexity with element swaps
 *
 * @code
 * // [10, 20, 30] becomes [30, 20, 10]
 * da_reverse(arr);
 * @endcode
 */
DA_DEF void da_reverse(da_array arr);

/**
 * @brief Swaps two elements at the specified indices
 * @param arr Array to modify (must not be NULL)
 * @param i First index (must be >= 0 and < length)
 * @param j Second index (must be >= 0 and < length)
 * @note Asserts on out-of-bounds indices
 * @note No-op if i == j
 *
 * @code
 * da_swap(arr, 0, 2);  // Swap first and third elements
 * @endcode
 */
DA_DEF void da_swap(da_array arr, int i, int j);

/**
 * @brief Checks if the array is empty
 * @param arr Array to check (must not be NULL)
 * @return 1 if length == 0, 0 otherwise
 * @note More readable than da_length(arr) == 0
 *
 * @code
 * if (da_is_empty(arr)) {
 *     printf("Array is empty\n");
 * }
 * @endcode
 */
DA_DEF int da_is_empty(da_array arr);

/**
 * @brief Find index of first element matching predicate
 * @param arr Array to search (must not be NULL)
 * @param predicate Function to test elements (must not be NULL)
 * @param context Optional context passed to predicate (can be NULL)
 * @return Index of first matching element, or -1 if not found
 * @note Predicate receives (element_pointer, context) and should return non-zero for match
 *
 * @code
 * int is_even(const void* elem, void* ctx) {
 *     return *(int*)elem % 2 == 0;
 * }
 * 
 * da_array numbers = DA_CREATE(int, 5);  // [1, 3, 4, 7, 8]
 * int index = da_find_index(numbers, is_even, NULL);  // returns 2 (index of 4)
 * @endcode
 */
DA_DEF int da_find_index(da_array arr, int (*predicate)(const void* element, void* context), void* context);

/**
 * @brief Check if array contains element matching predicate
 * @param arr Array to search (must not be NULL)
 * @param predicate Function to test elements (must not be NULL)
 * @param context Optional context passed to predicate (can be NULL)
 * @return 1 if matching element found, 0 otherwise
 * @note More readable than da_find_index(arr, predicate, context) != -1
 *
 * @code
 * int has_negative(const void* elem, void* ctx) {
 *     return *(int*)elem < 0;
 * }
 * 
 * if (da_contains(numbers, has_negative, NULL)) {
 *     printf("Array contains negative numbers\n");
 * }
 * @endcode
 */
DA_DEF int da_contains(da_array arr, int (*predicate)(const void* element, void* context), void* context);

/**
 * @brief Sort array elements using comparison function
 * @param arr Array to sort in-place (must not be NULL)
 * @param compare Comparison function (must not be NULL)
 * @param context Optional context passed to comparison function (can be NULL)
 * @note Comparison function signature: int (*compare)(const void* a, const void* b, void* context)
 * @note Should return <0 if a < b, 0 if a == b, >0 if a > b
 * @note Uses standard qsort algorithm
 *
 * @code
 * int compare_ints(const void* a, const void* b, void* ctx) {
 *     (void)ctx;
 *     int ia = *(const int*)a;
 *     int ib = *(const int*)b;
 *     return ia - ib;
 * }
 * 
 * da_sort(numbers, compare_ints, NULL);  // Sort ascending
 * @endcode
 */
DA_DEF void da_sort(da_array arr, int (*compare)(const void* a, const void* b, void* context), void* context);

/** @} */ // end of array_utility group

/**
 * @defgroup builder_lifecycle Builder Lifecycle
 * @brief Functions for creating and managing ArrayBuffer-style builders
 * @{
 */

/**
 * @brief Creates a new array builder for efficient construction
 * @param element_size Size in bytes of each element (must be > 0)
 * @return New builder with length = 0 and capacity = 0
 * @note Builders always use doubling growth strategy for fast construction
 * @note Not thread-safe
 * @note Use da_builder_to_array() to convert to ref-counted array
 * @note Asserts on allocation failure
 *
 * @code
 * da_builder builder = da_builder_create(sizeof(int));
 * DA_BUILDER_APPEND(builder, 42);
 * da_array arr = da_builder_to_array(&builder);
 * @endcode
 */
DA_DEF da_builder da_builder_create(int element_size);

/**
 * @brief Converts builder to a ref-counted array with exact capacity
 * @param builder Pointer to builder pointer (will be set to NULL)
 * @param destructor Optional destructor function for the resulting array (NULL if not needed)
 * @return New da_array with capacity = length (no wasted memory)
 * @note Builder is consumed and *builder is set to NULL
 * @note Resulting array has ref_count = 1
 * @note Memory is reallocated to exact size for efficiency
 * @note Perfect for microcontroller memory optimization
 *
 * @code
 * da_builder builder = DA_BUILDER_CREATE(int);
 * DA_BUILDER_APPEND(builder, 42);
 * da_array arr = da_builder_to_array(&builder, NULL);  // Simple types
 * 
 * // For complex types:
 * da_array cells = da_builder_to_array(&builder, (void(*)(void*))cell_release);
 * @endcode
 */
DA_DEF da_array da_builder_to_array(da_builder* builder, void (*retain_fn)(void*), void (*release_fn)(void*));

/**
 * @brief Removes all elements from the builder
 * @param builder Builder to clear (must not be NULL)
 * @note Sets length to 0 but preserves capacity
 * @note Allows reusing builder for multiple constructions
 *
 * @code
 * da_builder_clear(builder);
 * assert(da_builder_length(builder) == 0);
 * @endcode
 */
DA_DEF void da_builder_clear(da_builder builder);

/**
 * @brief Destroys a builder and frees its memory
 * @param builder Pointer to builder pointer (will be set to NULL)
 * @note Use this if you don't want to convert to da_array
 * @note Always sets *builder to NULL
 *
 * @code
 * da_builder builder = da_builder_create(sizeof(int));
 * // ... use builder ...
 * da_builder_destroy(&builder);  // builder becomes NULL
 * @endcode
 */
DA_DEF void da_builder_destroy(da_builder* builder);

/** @} */ // end of builder_lifecycle group

/**
 * @defgroup builder_modification Builder Modification
 * @brief Functions for adding elements to builders
 * @{
 */

/**
 * @brief Appends an element to the builder
 * @param builder Builder to modify (must not be NULL)
 * @param element Pointer to element data to copy (must not be NULL)
 * @note Always uses doubling growth strategy for fast construction
 * @note Asserts on allocation failure or NULL parameters
 * @note Much faster than da_push() for bulk construction
 *
 * @code
 * da_builder builder = DA_BUILDER_CREATE(int);
 * int value = 42;
 * da_builder_append(builder, &value);
 * @endcode
 */
DA_DEF void da_builder_append(da_builder builder, const void* element);

/**
 * @brief Ensures the builder has at least the specified capacity
 * @param builder Builder to modify (must not be NULL)
 * @param new_capacity Minimum capacity required (must be >= 0)
 * @note Only increases capacity, never decreases
 * @note Asserts on allocation failure
 * @note Useful for avoiding multiple reallocations when size is known
 * @note Builders always use doubling growth strategy
 *
 * @code
 * da_builder builder = DA_BUILDER_CREATE(int);
 * da_builder_reserve(builder, 1000);  // Ensure space for 1000 elements
 * for (int i = 0; i < 1000; i++) {
 *     DA_BUILDER_APPEND(builder, i);  // No reallocations needed
 * }
 * @endcode
 */
DA_DEF void da_builder_reserve(da_builder builder, int new_capacity);

/**
 * @brief Appends all elements from an array to the builder
 * @param builder Builder to modify (must not be NULL)
 * @param arr Source array to append from (must not be NULL)
 * @note Automatically grows builder capacity if needed
 * @note Source array is unchanged
 * @note Arrays must have the same element_size
 * @note More efficient than multiple da_builder_append() calls
 * @note Asserts on allocation failure or mismatched element sizes
 *
 * @code
 * da_array source = DA_CREATE(int, 3);  // [10, 20, 30]
 * da_builder builder = DA_BUILDER_CREATE(int);
 * da_builder_append_array(builder, source);  // Builder now contains [10, 20, 30]
 * @endcode
 */
DA_DEF void da_builder_append_array(da_builder builder, da_array arr);

/** @} */ // end of builder_modification group

/**
 * @defgroup builder_utility Builder Utility
 * @brief Functions for querying and accessing builder elements
 * @{
 */

/**
 * @brief Gets the current number of elements in the builder
 * @param builder Builder to query (must not be NULL)
 * @return Number of elements currently in the builder
 */
DA_DEF int da_builder_length(da_builder builder);

/**
 * @brief Gets the current allocated capacity of the builder
 * @param builder Builder to query (must not be NULL)
 * @return Number of elements that can be stored without reallocation
 */
DA_DEF int da_builder_capacity(da_builder builder);

/**
 * @brief Gets a pointer to an element at the specified index
 * @param builder Builder to access (must not be NULL)
 * @param index Element index (must be >= 0 and < length)
 * @return Pointer to element at index
 * @note Asserts on out-of-bounds access
 */
DA_DEF void* da_builder_get(da_builder builder, int index);

/**
 * @brief Sets the value of an element at the specified index
 * @param builder Builder to modify (must not be NULL)
 * @param index Element index (must be >= 0 and < length)
 * @param element Pointer to element data to copy (must not be NULL)
 * @note Asserts on out-of-bounds access or NULL parameters
 */
DA_DEF void da_builder_set(da_builder builder, int index, const void* element);

/** @} */ // end of builder_utility group

/**
 * @defgroup array_macros Type-Safe Array Macros
 * @brief Convenient type-safe macros for array operations
 * @{
 */

 /**
  * @def DA_CREATE(T, cap)
  * @brief Type-safe array creation
  * @param T Element type (e.g., int, float, struct mytype)
  * @param cap Initial capacity
  * @return New da_array sized for type T
  *
  * @code
  * da_array arr = DA_CREATE(int, 10);
  * @endcode
  */

 /**
  * @def DA_PUSH_TYPED(arr, val, T)
  * @brief Type-safe element append with explicit type parameter
  * @param arr Array to modify
  * @param val Value to append
  * @param T Explicit type of the value
  * @note Always requires explicit type parameter
  *
  * @code
  * DA_PUSH_TYPED(arr, 42, int);
  * @endcode
  */

 /**
  * @def DA_PUT_TYPED(arr, i, val, T)
  * @brief Type-safe element assignment with explicit type parameter
  * @param arr Array to modify
  * @param i Index to set
  * @param val Value to assign
  * @param T Explicit type of the value
  * @note Always requires explicit type parameter
  *
  * @code
  * DA_PUT_TYPED(arr, 0, 42, int);
  * @endcode
  */

 /**
  * @def DA_INSERT_TYPED(arr, i, val, T)
  * @brief Type-safe element insert with explicit type parameter
  * @param arr Array to modify
  * @param i Index to insert at
  * @param val Value to insert
  * @param T Explicit type of the value
  * @note Always requires explicit type parameter
  *
  * @code
  * DA_INSERT_TYPED(arr, 0, 42, int);
  * @endcode
  */

 /**
  * @def DA_PUSH_INFERRED(arr, val)
  * @brief Type-safe element append with automatic type inference
  * @param arr Array to modify
  * @param val Value to append
  * @note Only available when DA_SUPPORT_TYPE_INFERENCE=1
  * @note Automatically infers type using typeof or auto
  *
  * @code
  * DA_PUSH_INFERRED(arr, 42);  // Type inferred as int
  * @endcode
  */

 /**
  * @def DA_PUT_INFERRED(arr, i, val)
  * @brief Type-safe element assignment with automatic type inference
  * @param arr Array to modify
  * @param i Index to set
  * @param val Value to assign
  * @note Only available when DA_SUPPORT_TYPE_INFERENCE=1
  * @note Automatically infers type using typeof or auto
  *
  * @code
  * DA_PUT_INFERRED(arr, 0, 42);  // Type inferred as int
  * @endcode
  */

 /**
  * @def DA_INSERT_INFERRED(arr, i, val)
  * @brief Type-safe element insert with automatic type inference
  * @param arr Array to modify
  * @param i Index to insert at
  * @param val Value to insert
  * @note Only available when DA_SUPPORT_TYPE_INFERENCE=1
  * @note Automatically infers type using typeof or auto
  *
  * @code
  * DA_INSERT_INFERRED(arr, 0, 42);  // Type inferred as int
  * @endcode
  */

 /**
  * @def DA_PUSH(arr, val) or DA_PUSH(arr, val, T)
  * @brief Type-safe element append (adaptive macro)
  * @param arr Array to modify
  * @param val Value to append
  * @param T Type parameter (only when DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined)
  * @note When DA_SUPPORT_TYPE_INFERENCE=1 and DA_NOT_USE_TYPE_GENERIC is not defined: DA_PUSH(arr, 42)
  * @note When DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined: DA_PUSH(arr, 42, int)
  * @note Automatically chooses between inferred and typed versions
  *
  * @code
  * // With type inference support:
  * DA_PUSH(arr, 42);
  *
  * // Without type inference or with DA_NOT_USE_TYPE_GENERIC:
  * DA_PUSH(arr, 42, int);
  * @endcode
  */

 /**
  * @def DA_PUT(arr, i, val) or DA_PUT(arr, i, val, T)
  * @brief Type-safe element assignment (adaptive macro)
  * @param arr Array to modify
  * @param i Index to set
  * @param val Value to assign
  * @param T Type parameter (only when DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined)
  * @note When DA_SUPPORT_TYPE_INFERENCE=1 and DA_NOT_USE_TYPE_GENERIC is not defined: DA_PUT(arr, 0, 42)
  * @note When DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined: DA_PUT(arr, 0, 42, int)
  * @note Automatically chooses between inferred and typed versions
  *
  * @code
  * // With type inference support:
  * DA_PUT(arr, 0, 42);
  *
  * // Without type inference or with DA_NOT_USE_TYPE_GENERIC:
  * DA_PUT(arr, 0, 42, int);
  * @endcode
  */

 /**
  * @def DA_INSERT(arr, i, val) or DA_INSERT(arr, i, val, T)
  * @brief Type-safe element insert (adaptive macro)
  * @param arr Array to modify
  * @param i Index to insert at
  * @param val Value to insert
  * @param T Type parameter (only when DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined)
  * @note When DA_SUPPORT_TYPE_INFERENCE=1 and DA_NOT_USE_TYPE_GENERIC is not defined: DA_INSERT(arr, 0, 42)
  * @note When DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined: DA_INSERT(arr, 0, 42, int)
  * @note Automatically chooses between inferred and typed versions
  *
  * @code
  * // With type inference support:
  * DA_INSERT(arr, 0, 42);
  *
  * // Without type inference or with DA_NOT_USE_TYPE_GENERIC:
  * DA_INSERT(arr, 0, 42, int);
  * @endcode
  */

/**
 * @def DA_AT(arr, i, T)
 * @brief Type-safe element access by value
 * @param arr Array to access
 * @param i Index to get
 * @param T Element type
 * @return Element value at index i
 *
 * @code
 * int value = DA_AT(arr, 0, int);
 * @endcode
 */

/**
 * @def DA_LENGTH(arr)
 * @brief Get array length (shorthand for da_length)
 */

/**
 * @def DA_CAPACITY(arr)
 * @brief Get array capacity (shorthand for da_capacity)
 */

/**
 * @def DA_POP(arr, out_ptr)
 * @brief Pop last element (shorthand for da_pop)
 */

/**
 * @def DA_CLEAR(arr)
 * @brief Clear array (shorthand for da_clear)
 */

/**
 * @def DA_RESERVE(arr, cap)
 * @brief Reserve capacity (shorthand for da_reserve)
 */

/**
 * @def DA_RESIZE(arr, len)
 * @brief Resize array (shorthand for da_resize)
 */

/**
 * @def DA_INSERT(arr, i, val)
 * @brief Type-safe element insert (with typeof support)
 * @param arr Array to modify
 * @param i Index to insert at
 * @param val Value to insert
 * @note With typeof support: DA_INSERT(arr, 0, 42)
 * @note Without typeof: DA_INSERT(arr, 0, 42, int)
 */

/**
 * @def DA_REMOVE(arr, i, out_ptr)
 * @brief Remove element at index (shorthand for da_remove)
 * @param arr Array to modify
 * @param i Index to remove from
 * @param out_ptr Optional pointer to store removed element (can be NULL)
 */

/** @} */ // end of array_macros group

#define DA_NEW(T) da_new(sizeof(T))
#define DA_CREATE(T, cap, retain_fn, release_fn) da_create(sizeof(T), cap, retain_fn, release_fn)

#define DA_PUSH_TYPED(arr, val, T) do { T _temp = (val); da_push(arr, (void*)&_temp); } while(0)
#define DA_PUT_TYPED(arr, i, val, T) do { T _temp = (val); da_set(arr, i, (void*)&_temp); } while(0)
#define DA_INSERT_TYPED(arr, i, val, T) do { T _temp = (val); da_insert(arr, i, (void*)&_temp); } while(0)

#if DA_SUPPORT_TYPE_INFERENCE
    #define DA_PUSH_INFERRED(arr, val) do { DA_MAKE_VAR_WITH_INFERRED_TYPE(_temp, val); da_push(arr, (void*)&_temp); } while(0)
    #define DA_PUT_INFERRED(arr, i, val) do { DA_MAKE_VAR_WITH_INFERRED_TYPE(_temp, val); da_set(arr, i, (void*)&_temp); } while(0)
    #define DA_INSERT_INFERRED(arr, i, val) do { DA_MAKE_VAR_WITH_INFERRED_TYPE(_temp, val); da_insert(arr, i, (void*)&_temp); } while(0)
#endif

#if DA_SUPPORT_TYPE_INFERENCE && !defined(DA_NOT_USE_TYPE_GENERIC)
    #define DA_PUSH(arr, val) DA_PUSH_INFERRED(arr, val)
    #define DA_PUT(arr, i, val) DA_PUT_INFERRED(arr, i, val)
    #define DA_INSERT(arr, i, val) DA_INSERT_INFERRED(arr, i, val)
#else
    #define DA_PUSH(arr, val, T) DA_PUSH_TYPED(arr, val, T)
    #define DA_PUT(arr, i, val, T) DA_PUT_TYPED(arr, i, val, T)
    #define DA_INSERT(arr, i, val, T) DA_INSERT_TYPED(arr, i, val, T)
#endif

#define DA_LENGTH(arr) da_length(arr)
#define DA_CAPACITY(arr) da_capacity(arr)
#define DA_AT(arr, i, T) (*(T*)da_get(arr, i))
#define DA_POP(arr, out_ptr) da_pop(arr, out_ptr)
#define DA_REMOVE(arr, i, out_ptr) da_remove(arr, i, out_ptr)
#define DA_CLEAR(arr) da_clear(arr)
#define DA_RESERVE(arr, cap) da_reserve(arr, cap)
#define DA_RESIZE(arr, len) da_resize(arr, len)
#define DA_TRIM(arr, cap) da_trim(arr, cap)
#define DA_SHRINK_TO_FIT(arr) da_trim(arr, da_length(arr))
#define DA_PEEK(arr, T) (*(T*)da_peek(arr))
#define DA_PEEK_FIRST(arr, T) (*(T*)da_peek_first(arr))

/**
 * @defgroup builder_macros Type-Safe Builder Macros
 * @brief Convenient type-safe macros for builder operations
 * @{
 */

 /**
  * @def DA_BUILDER_CREATE(T)
  * @brief Type-safe builder creation
  * @param T Element type (e.g., int, float, struct mytype)
  * @return New da_builder sized for type T
  *
  * @code
  * da_builder builder = DA_BUILDER_CREATE(int);
  * @endcode
  */

 /**
  * @def DA_BUILDER_APPEND_TYPED(builder, val, T)
  * @brief Type-safe element append to builder with explicit type parameter
  * @param builder Builder to modify
  * @param val Value to append
  * @param T Explicit type of the value
  * @note Always requires explicit type parameter
  *
  * @code
  * DA_BUILDER_APPEND_TYPED(builder, 42, int);
  * @endcode
  */

 /**
  * @def DA_BUILDER_PUT_TYPED(builder, i, val, T)
  * @brief Type-safe element assignment in builder with explicit type parameter
  * @param builder Builder to modify
  * @param i Index to set
  * @param val Value to assign
  * @param T Explicit type of the value
  * @note Always requires explicit type parameter
  *
  * @code
  * DA_BUILDER_PUT_TYPED(builder, 0, 42, int);
  * @endcode
  */

 /**
  * @def DA_BUILDER_APPEND_INFERRED(builder, val)
  * @brief Type-safe element append to builder with automatic type inference
  * @param builder Builder to modify
  * @param val Value to append
  * @note Only available when DA_SUPPORT_TYPE_INFERENCE=1
  * @note Automatically infers type using typeof or auto
  *
  * @code
  * DA_BUILDER_APPEND_INFERRED(builder, 42);  // Type inferred as int
  * @endcode
  */

 /**
  * @def DA_BUILDER_PUT_INFERRED(builder, i, val)
  * @brief Type-safe element assignment in builder with automatic type inference
  * @param builder Builder to modify
  * @param i Index to set
  * @param val Value to assign
  * @note Only available when DA_SUPPORT_TYPE_INFERENCE=1
  * @note Automatically infers type using typeof or auto
  *
  * @code
  * DA_BUILDER_PUT_INFERRED(builder, 0, 42);  // Type inferred as int
  * @endcode
  */

 /**
  * @def DA_BUILDER_APPEND(builder, val) or DA_BUILDER_APPEND(builder, val, T)
  * @brief Type-safe element append to builder (adaptive macro)
  * @param builder Builder to modify
  * @param val Value to append
  * @param T Type parameter (only when DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined)
  * @note When DA_SUPPORT_TYPE_INFERENCE=1 and DA_NOT_USE_TYPE_GENERIC is not defined: DA_BUILDER_APPEND(builder, 42)
  * @note When DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined: DA_BUILDER_APPEND(builder, 42, int)
  * @note Automatically chooses between inferred and typed versions
  *
  * @code
  * // With type inference support:
  * DA_BUILDER_APPEND(builder, 42);
  *
  * // Without type inference or with DA_NOT_USE_TYPE_GENERIC:
  * DA_BUILDER_APPEND(builder, 42, int);
  * @endcode
  */

 /**
  * @def DA_BUILDER_PUT(builder, i, val) or DA_BUILDER_PUT(builder, i, val, T)
  * @brief Type-safe element assignment in builder (adaptive macro)
  * @param builder Builder to modify
  * @param i Index to set
  * @param val Value to assign
  * @param T Type parameter (only when DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined)
  * @note When DA_SUPPORT_TYPE_INFERENCE=1 and DA_NOT_USE_TYPE_GENERIC is not defined: DA_BUILDER_PUT(builder, 0, 42)
  * @note When DA_SUPPORT_TYPE_INFERENCE=0 or DA_NOT_USE_TYPE_GENERIC is defined: DA_BUILDER_PUT(builder, 0, 42, int)
  * @note Automatically chooses between inferred and typed versions
  *
  * @code
  * // With type inference support:
  * DA_BUILDER_PUT(builder, 0, 42);
  *
  * // Without type inference or with DA_NOT_USE_TYPE_GENERIC:
  * DA_BUILDER_PUT(builder, 0, 42, int);
  * @endcode
  */

/**
 * @def DA_BUILDER_AT(builder, i, T)
 * @brief Type-safe element access by value from builder
 * @param builder Builder to access
 * @param i Index to get
 * @param T Element type
 * @return Element value at index i
 *
 * @code
 * int value = DA_BUILDER_AT(builder, 0, int);
 * @endcode
 */

/**
 * @def DA_BUILDER_LEN(builder)
 * @brief Get builder length (shorthand for da_builder_length)
 */

/**
 * @def DA_BUILDER_CAP(builder)
 * @brief Get builder capacity (shorthand for da_builder_capacity)
 */

/**
 * @def DA_BUILDER_CLEAR(builder)
 * @brief Clear builder (shorthand for da_builder_clear)
 */

/**
 * @def DA_BUILDER_TO_ARRAY(builder)
 * @brief Convert builder to array (shorthand for da_builder_to_array)
 */

#define DA_BUILDER_CREATE(T) da_builder_create(sizeof(T))

#define DA_BUILDER_APPEND_TYPED(builder, val, T) do { T _temp = (val); da_builder_append(builder, (void*)&_temp); } while(0)
#define DA_BUILDER_PUT_TYPED(builder, i, val, T) do { T _temp = (val); da_builder_set(builder, i, (void*)&_temp); } while(0)

#if DA_SUPPORT_TYPE_INFERENCE
    #define DA_BUILDER_APPEND_INFERRED(builder, val) \
        do { DA_MAKE_VAR_WITH_INFERRED_TYPE(_temp, val); da_builder_append(builder, (void*)&_temp); } while(0)
    #define DA_BUILDER_PUT_INFERRED(builder, i, val) \
        do { DA_MAKE_VAR_WITH_INFERRED_TYPE(_temp, val); da_builder_set(builder, i, (void*)&_temp); } while(0)
#endif

#if DA_SUPPORT_TYPE_INFERENCE && !defined(DA_NOT_USE_TYPE_GENERIC)
    #define DA_BUILDER_APPEND(builder, val) DA_BUILDER_APPEND_INFERRED(builder, val)
    #define DA_BUILDER_PUT(builder, i, val) DA_BUILDER_PUT_INFERRED(builder, i, val)
#else
    #define DA_BUILDER_APPEND(builder, val, T) DA_BUILDER_APPEND_TYPED(builder, val, T)
    #define DA_BUILDER_PUT(builder, i, val, T) DA_BUILDER_PUT_TYPED(builder, i, val, T)
#endif
/** @} */ // end of builder_macros group


#define DA_BUILDER_LEN(builder) da_builder_length(builder)
#define DA_BUILDER_CAP(builder) da_builder_capacity(builder)
#define DA_BUILDER_AT(builder, i, T) (*(T*)da_builder_get(builder, i))
#define DA_BUILDER_CLEAR(builder) da_builder_clear(builder)
#define DA_BUILDER_TO_ARRAY(builder) da_builder_to_array(builder, NULL, NULL)
#define DA_BUILDER_TO_ARRAY_MANAGED(builder, retain_fn, release_fn) da_builder_to_array(builder, retain_fn, release_fn)

/* Implementation */
#ifdef DA_IMPLEMENTATION

static int da_grow_capacity(int current_capacity, int min_needed) {
    int new_capacity = current_capacity;

#ifdef DA_GROWTH
    /* Fixed growth strategy */
    while (new_capacity < min_needed) {
        new_capacity += DA_GROWTH;
    }
#else
    /* Doubling strategy */
    if (new_capacity == 0) new_capacity = 1;
    while (new_capacity < min_needed) {
        new_capacity *= 2;
    }
#endif

    return new_capacity;
}

static int da_builder_grow_capacity(int current_capacity, int min_needed) {
    /* Builders always use doubling strategy for fast construction */
    int new_capacity = current_capacity;
    if (new_capacity == 0) new_capacity = 1;
    while (new_capacity < min_needed) {
        new_capacity *= 2;
    }
    return new_capacity;
}

/* Array Implementation */

DA_DEF da_array da_new(int element_size) {
    DA_ASSERT(element_size > 0);

    da_array arr = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(arr != NULL);

    DA_ATOMIC_STORE(&arr->ref_count, 1);
    arr->length = 0;
    arr->capacity = 0;  /* Deferred allocation */
    arr->element_size = element_size;
    arr->retain_fn = NULL;
    arr->release_fn = NULL;
    arr->data = NULL;

    return arr;
}

DA_DEF da_array da_create(int element_size, int initial_capacity, void (*retain_fn)(void*), void (*release_fn)(void*)) {
    DA_ASSERT(element_size > 0);
    DA_ASSERT(initial_capacity >= 0);

    da_array arr = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(arr != NULL);

    DA_ATOMIC_STORE(&arr->ref_count, 1);
    arr->length = 0;
    arr->capacity = initial_capacity;
    arr->element_size = element_size;
    arr->retain_fn = retain_fn;
    arr->release_fn = release_fn;

    if (initial_capacity > 0) {
        arr->data = DA_MALLOC(initial_capacity * element_size);
        DA_ASSERT(arr->data != NULL);
    } else {
        arr->data = NULL;
    }

    return arr;
}

DA_DEF void da_release(da_array* arr) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(*arr != NULL);

    int old_count = DA_ATOMIC_FETCH_SUB(&(*arr)->ref_count, 1);

    if (old_count == 1) {  /* We were the last reference */
        if ((*arr)->data && (*arr)->release_fn) {
            /* Call release function on each element before freeing */
            for (int i = 0; i < (*arr)->length; i++) {
                void* element_ptr = (char*)(*arr)->data + (i * (*arr)->element_size);
                (*arr)->release_fn(element_ptr);
            }
        }
        if ((*arr)->data) {
            DA_FREE((*arr)->data);
        }
        DA_FREE(*arr);
    }

    *arr = NULL;  /* Always NULL the pointer for safety */
}

DA_DEF da_array da_retain(da_array arr) {
    DA_ASSERT(arr != NULL);
    DA_ATOMIC_FETCH_ADD(&arr->ref_count, 1);
    return arr;
}

DA_DEF void* da_get(da_array arr, int index) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(index >= 0 && index < arr->length);
    return (char*)arr->data + (index * arr->element_size);
}

DA_DEF void* da_data(da_array arr) {
    DA_ASSERT(arr != NULL);
    return arr->data;
}

DA_DEF void da_set(da_array arr, int index, const void* element) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(element != NULL);
    DA_ASSERT(index >= 0 && index < arr->length);

    void* dest = (char*)arr->data + (index * arr->element_size);
    
    /* Call release function on the old element before overwriting */
    if (arr->release_fn) {
        arr->release_fn(dest);
    }
    
    memcpy(dest, element, arr->element_size);
    
    /* Call retain function on the newly set element */
    if (arr->retain_fn) {
        arr->retain_fn(dest);
    }
}

DA_DEF void da_push(da_array arr, const void* element) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(element != NULL);

    if (arr->length >= arr->capacity) {
        int new_capacity = da_grow_capacity(arr->capacity, arr->length + 1);
        arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
        DA_ASSERT(arr->data != NULL);
        arr->capacity = new_capacity;
    }

    void* dest = (char*)arr->data + (arr->length * arr->element_size);
    memcpy(dest, element, arr->element_size);
    
    /* Call retain function on the newly added element */
    if (arr->retain_fn) {
        arr->retain_fn(dest);
    }
    
    arr->length++;
}

DA_DEF void da_insert(da_array arr, int index, const void* element) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(element != NULL);
    DA_ASSERT(index >= 0 && index <= arr->length);

    /* Grow array if needed */
    if (arr->length >= arr->capacity) {
        int new_capacity = da_grow_capacity(arr->capacity, arr->length + 1);
        arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
        DA_ASSERT(arr->data != NULL);
        arr->capacity = new_capacity;
    }

    /* Shift elements to the right if not inserting at the end */
    if (index < arr->length) {
        void* src = (char*)arr->data + (index * arr->element_size);
        void* dest = (char*)arr->data + ((index + 1) * arr->element_size);
        int bytes_to_move = (arr->length - index) * arr->element_size;
        memmove(dest, src, bytes_to_move);
    }

    /* Insert the new element */
    void* insert_pos = (char*)arr->data + (index * arr->element_size);
    memcpy(insert_pos, element, arr->element_size);
    
    /* Call retain function on the newly inserted element */
    if (arr->retain_fn) {
        arr->retain_fn(insert_pos);
    }
    
    arr->length++;
}

DA_DEF void da_remove(da_array arr, int index, void* out) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(index >= 0 && index < arr->length);

    void* element_ptr = (char*)arr->data + (index * arr->element_size);
    
    /* Copy element to output if requested */
    if (out != NULL) {
        memcpy(out, element_ptr, arr->element_size);
    }
    
    /* Call destructor on the removed element */
    if (arr->release_fn) {
        arr->release_fn(element_ptr);
    }

    /* Shift elements to the left if not removing the last element */
    if (index < arr->length - 1) {
        void* dest = (char*)arr->data + (index * arr->element_size);
        void* src = (char*)arr->data + ((index + 1) * arr->element_size);
        int bytes_to_move = (arr->length - index - 1) * arr->element_size;
        memmove(dest, src, bytes_to_move);
    }

    arr->length--;
}

DA_DEF void da_pop(da_array arr, void* out) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(arr->length > 0);

    arr->length--;

    void* src = (char*)arr->data + (arr->length * arr->element_size);
    if (out != NULL) {
        memcpy(out, src, arr->element_size);
    }
    
    /* Call release function on the popped element */
    if (arr->release_fn) {
        arr->release_fn(src);
    }
}

DA_DEF void da_clear(da_array arr) {
    DA_ASSERT(arr != NULL);
    
    /* Call release function on all elements before clearing */
    if (arr->release_fn && arr->data) {
        for (int i = 0; i < arr->length; i++) {
            void* element_ptr = (char*)arr->data + (i * arr->element_size);
            arr->release_fn(element_ptr);
        }
    }
    
    arr->length = 0;
}

DA_DEF int da_length(da_array arr) {
    DA_ASSERT(arr != NULL);
    return arr->length;
}

DA_DEF int da_capacity(da_array arr) {
    DA_ASSERT(arr != NULL);
    return arr->capacity;
}

DA_DEF void da_reserve(da_array arr, int new_capacity) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(new_capacity >= 0);

    if (new_capacity > arr->capacity) {
        arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
        DA_ASSERT(arr->data != NULL);
        arr->capacity = new_capacity;
    }
}

DA_DEF void da_resize(da_array arr, int new_length) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(new_length >= 0);

    if (new_length > arr->capacity) {
        da_reserve(arr, new_length);
    }

    if (new_length < arr->length) {
        /* Call destructor on elements being removed */
        if (arr->release_fn && arr->data) {
            for (int i = new_length; i < arr->length; i++) {
                void* element_ptr = (char*)arr->data + (i * arr->element_size);
                arr->release_fn(element_ptr);
            }
        }
    } else if (new_length > arr->length) {
        /* Zero-fill new elements */
        void* start = (char*)arr->data + (arr->length * arr->element_size);
        int bytes_to_zero = (new_length - arr->length) * arr->element_size;
        memset(start, 0, bytes_to_zero);
    }

    arr->length = new_length;
}

DA_DEF void da_trim(da_array arr, int new_capacity) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(new_capacity >= arr->length);

    if (new_capacity < arr->capacity) {
        if (new_capacity == 0) {
            if (arr->data) {
                DA_FREE(arr->data);
                arr->data = NULL;
            }
        } else {
            arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
            DA_ASSERT(arr->data != NULL);
        }
        arr->capacity = new_capacity;
    }
}

DA_DEF void da_append_array(da_array dest, da_array src) {
    DA_ASSERT(dest != NULL);
    DA_ASSERT(src != NULL);
    DA_ASSERT(dest->element_size == src->element_size);

    if (src->length == 0) return;  /* Nothing to append */

    /* Ensure dest has enough capacity */
    int new_length = dest->length + src->length;
    if (new_length > dest->capacity) {
        int new_capacity = da_grow_capacity(dest->capacity, new_length);
        dest->data = DA_REALLOC(dest->data, new_capacity * dest->element_size);
        DA_ASSERT(dest->data != NULL);
        dest->capacity = new_capacity;
    }

    /* Copy all elements from src to end of dest */
    void* dest_ptr = (char*)dest->data + (dest->length * dest->element_size);
    memcpy(dest_ptr, src->data, src->length * src->element_size);
    
    /* Call retain function on all copied elements */
    if (dest->retain_fn) {
        for (int i = dest->length; i < new_length; i++) {
            void* element_ptr = (char*)dest->data + (i * dest->element_size);
            dest->retain_fn(element_ptr);
        }
    }
    
    dest->length = new_length;
}

DA_DEF da_array da_concat(da_array arr1, da_array arr2) {
    DA_ASSERT(arr1 != NULL);
    DA_ASSERT(arr2 != NULL);
    DA_ASSERT(arr1->element_size == arr2->element_size);

    int total_length = arr1->length + arr2->length;

    /* Create new array with exact capacity */
    da_array result = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(result != NULL);

    DA_ATOMIC_STORE(&result->ref_count, 1);
    result->length = total_length;
    result->capacity = total_length;  /* Exact capacity */
    result->element_size = arr1->element_size;
    result->retain_fn = arr1->retain_fn;   /* Inherit retain function from first array */
    result->release_fn = arr1->release_fn;  /* Inherit release function from first array */

    if (total_length > 0) {
        result->data = DA_MALLOC(total_length * result->element_size);
        DA_ASSERT(result->data != NULL);

        /* Copy arr1 elements first */
        if (arr1->length > 0) {
            memcpy(result->data, arr1->data, arr1->length * result->element_size);
        }

        /* Copy arr2 elements after arr1 */
        if (arr2->length > 0) {
            void* dest_ptr = (char*)result->data + (arr1->length * result->element_size);
            memcpy(dest_ptr, arr2->data, arr2->length * result->element_size);
        }
        
        /* Call retain function on all copied elements */
        if (result->retain_fn) {
            for (int i = 0; i < result->length; i++) {
                void* element_ptr = (char*)result->data + (i * result->element_size);
                result->retain_fn(element_ptr);
            }
        }
    } else {
        result->data = NULL;
    }

    return result;
}

/* Builder Implementation */

DA_DEF da_builder da_builder_create(int element_size) {
    DA_ASSERT(element_size > 0);

    da_builder builder = (da_builder)DA_MALLOC(sizeof(da_builder_t));
    DA_ASSERT(builder != NULL);

    builder->length = 0;
    builder->capacity = 0;
    builder->element_size = element_size;
    builder->data = NULL;

    return builder;
}

DA_DEF void da_builder_append(da_builder builder, const void* element) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(element != NULL);

    if (builder->length >= builder->capacity) {
        int new_capacity = da_builder_grow_capacity(builder->capacity, builder->length + 1);
        builder->data = DA_REALLOC(builder->data, new_capacity * builder->element_size);
        DA_ASSERT(builder->data != NULL);
        builder->capacity = new_capacity;
    }

    void* dest = (char*)builder->data + (builder->length * builder->element_size);
    memcpy(dest, element, builder->element_size);
    builder->length++;
}

DA_DEF void da_builder_reserve(da_builder builder, int new_capacity) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(new_capacity >= 0);

    if (new_capacity > builder->capacity) {
        builder->data = DA_REALLOC(builder->data, new_capacity * builder->element_size);
        DA_ASSERT(builder->data != NULL);
        builder->capacity = new_capacity;
    }
}

DA_DEF void da_builder_append_array(da_builder builder, da_array arr) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(arr != NULL);
    DA_ASSERT(builder->element_size == arr->element_size);

    if (arr->length == 0) return;  /* Nothing to append */

    /* Ensure enough capacity */
    int new_length = builder->length + arr->length;
    if (new_length > builder->capacity) {
        int new_capacity = da_builder_grow_capacity(builder->capacity, new_length);
        builder->data = DA_REALLOC(builder->data, new_capacity * builder->element_size);
        DA_ASSERT(builder->data != NULL);
        builder->capacity = new_capacity;
    }

    /* Copy all elements from array at once */
    void* dest_ptr = (char*)builder->data + (builder->length * builder->element_size);
    memcpy(dest_ptr, arr->data, arr->length * arr->element_size);
    builder->length = new_length;
}

DA_DEF da_array da_builder_to_array(da_builder* builder, void (*retain_fn)(void*), void (*release_fn)(void*)) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(*builder != NULL);

    da_builder b = *builder;

    /* Create new da_array */
    da_array arr = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(arr != NULL);

    DA_ATOMIC_STORE(&arr->ref_count, 1);
    arr->length = b->length;
    arr->capacity = b->length;  /* Exact capacity = length */
    arr->element_size = b->element_size;
    arr->retain_fn = retain_fn;
    arr->release_fn = release_fn;

    if (b->length > 0) {
        /* Shrink to exact size */
        arr->data = DA_REALLOC(b->data, b->length * b->element_size);
        DA_ASSERT(arr->data != NULL);
        
        /* Call retain function on all elements in the new array */
        if (arr->retain_fn) {
            for (int i = 0; i < arr->length; i++) {
                void* element_ptr = (char*)arr->data + (i * arr->element_size);
                arr->retain_fn(element_ptr);
            }
        }
    } else {
        arr->data = NULL;
        if (b->data) {
            DA_FREE(b->data);
        }
    }

    /* Free builder */
    DA_FREE(b);
    *builder = NULL;

    return arr;
}

DA_DEF void da_builder_clear(da_builder builder) {
    DA_ASSERT(builder != NULL);
    builder->length = 0;
}

DA_DEF void da_builder_destroy(da_builder* builder) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(*builder != NULL);

    if ((*builder)->data) {
        DA_FREE((*builder)->data);
    }
    DA_FREE(*builder);
    *builder = NULL;
}

DA_DEF int da_builder_length(da_builder builder) {
    DA_ASSERT(builder != NULL);
    return builder->length;
}

DA_DEF int da_builder_capacity(da_builder builder) {
    DA_ASSERT(builder != NULL);
    return builder->capacity;
}

DA_DEF void* da_builder_get(da_builder builder, int index) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(index >= 0 && index < builder->length);
    return (char*)builder->data + (index * builder->element_size);
}

DA_DEF void da_builder_set(da_builder builder, int index, const void* element) {
    DA_ASSERT(builder != NULL);
    DA_ASSERT(element != NULL);
    DA_ASSERT(index >= 0 && index < builder->length);

    void* dest = (char*)builder->data + (index * builder->element_size);
    memcpy(dest, element, builder->element_size);
}

/* Additional Array Functions Implementation */

DA_DEF void* da_peek(da_array arr) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(arr->length > 0);
    return (char*)arr->data + ((arr->length - 1) * arr->element_size);
}

DA_DEF void* da_peek_first(da_array arr) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(arr->length > 0);
    return arr->data;
}

DA_DEF void da_append_raw(da_array arr, const void* data, int count) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(data != NULL);
    DA_ASSERT(count >= 0);

    if (count == 0) return;  /* Nothing to append */

    /* Ensure enough capacity */
    int new_length = arr->length + count;
    if (new_length > arr->capacity) {
        int new_capacity = da_grow_capacity(arr->capacity, new_length);
        arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
        DA_ASSERT(arr->data != NULL);
        arr->capacity = new_capacity;
    }

    /* Copy all elements at once */
    void* dest_ptr = (char*)arr->data + (arr->length * arr->element_size);
    memcpy(dest_ptr, data, count * arr->element_size);
    
    /* Call retain function on all copied elements */
    if (arr->retain_fn) {
        for (int i = arr->length; i < new_length; i++) {
            void* element_ptr = (char*)arr->data + (i * arr->element_size);
            arr->retain_fn(element_ptr);
        }
    }
    
    arr->length = new_length;
}

DA_DEF void da_fill(da_array arr, const void* element, int count) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(element != NULL);
    DA_ASSERT(count >= 0);

    if (count == 0) return;  /* Nothing to fill */

    /* Ensure enough capacity */
    int new_length = arr->length + count;
    if (new_length > arr->capacity) {
        int new_capacity = da_grow_capacity(arr->capacity, new_length);
        arr->data = DA_REALLOC(arr->data, new_capacity * arr->element_size);
        DA_ASSERT(arr->data != NULL);
        arr->capacity = new_capacity;
    }

    /* Fill elements one by one */
    for (int i = 0; i < count; i++) {
        void* dest_ptr = (char*)arr->data + ((arr->length + i) * arr->element_size);
        memcpy(dest_ptr, element, arr->element_size);
    }
    arr->length = new_length;
}

DA_DEF da_array da_slice(da_array arr, int start, int end) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(start >= 0 && start <= arr->length);
    DA_ASSERT(end >= start && end <= arr->length);

    int slice_length = end - start;

    /* Create new array with exact capacity */
    da_array result = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(result != NULL);

    DA_ATOMIC_STORE(&result->ref_count, 1);
    result->length = slice_length;
    result->capacity = slice_length;  /* Exact capacity */
    result->element_size = arr->element_size;
    result->retain_fn = arr->retain_fn;   /* Inherit retain function */
    result->release_fn = arr->release_fn;  /* Inherit release function */

    if (slice_length > 0) {
        result->data = DA_MALLOC(slice_length * result->element_size);
        DA_ASSERT(result->data != NULL);

        /* Copy slice elements */
        void* src_ptr = (char*)arr->data + (start * arr->element_size);
        memcpy(result->data, src_ptr, slice_length * arr->element_size);
        
        /* Call retain function on all copied elements */
        if (result->retain_fn) {
            for (int i = 0; i < result->length; i++) {
                void* element_ptr = (char*)result->data + (i * result->element_size);
                result->retain_fn(element_ptr);
            }
        }
    } else {
        result->data = NULL;
    }

    return result;
}

DA_DEF void da_remove_range(da_array arr, int start, int count) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(start >= 0 && start < arr->length);
    DA_ASSERT(count >= 0);
    DA_ASSERT(start + count <= arr->length);

    if (count == 0) return;  /* Nothing to remove */

    int end = start + count;

    /* Call destructor on elements being removed */
    if (arr->release_fn) {
        for (int i = start; i < end; i++) {
            void* element_ptr = (char*)arr->data + (i * arr->element_size);
            arr->release_fn(element_ptr);
        }
    }

    /* Shift elements after the range to the left */
    if (end < arr->length) {
        void* dest = (char*)arr->data + (start * arr->element_size);
        void* src = (char*)arr->data + (end * arr->element_size);
        int bytes_to_move = (arr->length - end) * arr->element_size;
        memmove(dest, src, bytes_to_move);
    }

    arr->length -= count;
}

DA_DEF void da_reverse(da_array arr) {
    DA_ASSERT(arr != NULL);

    if (arr->length <= 1) return;  /* Nothing to reverse */

    char* temp = (char*)DA_MALLOC(arr->element_size);
    DA_ASSERT(temp != NULL);

    /* Swap elements from both ends moving toward center */
    for (int i = 0; i < arr->length / 2; i++) {
        int j = arr->length - 1 - i;

        char* left = (char*)arr->data + (i * arr->element_size);
        char* right = (char*)arr->data + (j * arr->element_size);

        /* Three-way swap using temp buffer */
        memcpy(temp, left, arr->element_size);
        memcpy(left, right, arr->element_size);
        memcpy(right, temp, arr->element_size);
    }

    DA_FREE(temp);
}

DA_DEF void da_swap(da_array arr, int i, int j) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(i >= 0 && i < arr->length);
    DA_ASSERT(j >= 0 && j < arr->length);

    if (i == j) return;  /* No-op if same index */

    char* temp = (char*)DA_MALLOC(arr->element_size);
    DA_ASSERT(temp != NULL);

    char* elem_i = (char*)arr->data + (i * arr->element_size);
    char* elem_j = (char*)arr->data + (j * arr->element_size);

    /* Three-way swap */
    memcpy(temp, elem_i, arr->element_size);
    memcpy(elem_i, elem_j, arr->element_size);
    memcpy(elem_j, temp, arr->element_size);

    DA_FREE(temp);
}

DA_DEF da_array da_copy(da_array arr) {
    DA_ASSERT(arr != NULL);

    /* Create new array with exact capacity = length */
    da_array result = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(result != NULL);

    DA_ATOMIC_STORE(&result->ref_count, 1);
    result->length = arr->length;
    result->capacity = arr->length;  /* Exact capacity for efficiency */
    result->element_size = arr->element_size;
    result->retain_fn = arr->retain_fn;   /* Inherit retain function */
    result->release_fn = arr->release_fn;  /* Inherit release function */

    if (arr->length > 0) {
        result->data = DA_MALLOC(arr->length * arr->element_size);
        DA_ASSERT(result->data != NULL);

        /* Copy all elements */
        memcpy(result->data, arr->data, arr->length * arr->element_size);
        
        /* Call retain function on all copied elements */
        if (result->retain_fn) {
            for (int i = 0; i < result->length; i++) {
                void* element_ptr = (char*)result->data + (i * result->element_size);
                result->retain_fn(element_ptr);
            }
        }
    } else {
        result->data = NULL;
    }

    return result;
}

DA_DEF da_array da_filter(da_array arr, int (*predicate)(const void* element, void* context), void* context) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(predicate != NULL);

    /* Use builder for single-pass filtering */
    da_builder builder = da_builder_create(arr->element_size);

    /* Single pass: test and append matching elements */
    for (int i = 0; i < arr->length; i++) {
        void* element_ptr = (char*)arr->data + (i * arr->element_size);
        if (predicate(element_ptr, context)) {
            da_builder_append(builder, element_ptr);
        }
    }

    /* Convert builder to array with exact capacity, inherit destructor */
    da_array result = da_builder_to_array(&builder, arr->retain_fn, arr->release_fn);
    return result;
}

DA_DEF da_array da_map(da_array arr, void (*mapper)(const void* src, void* dst, void* context), void* context) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(mapper != NULL);

    /* Create new array with same length and exact capacity */
    da_array result = (da_array)DA_MALLOC(sizeof(da_array_t));
    DA_ASSERT(result != NULL);

    DA_ATOMIC_STORE(&result->ref_count, 1);
    result->length = arr->length;
    result->capacity = arr->length;  /* Exact capacity for efficiency */
    result->element_size = arr->element_size;
    result->retain_fn = arr->retain_fn;   /* Inherit retain function */
    result->release_fn = arr->release_fn;  /* Inherit release function */

    if (arr->length > 0) {
        result->data = DA_MALLOC(arr->length * arr->element_size);
        DA_ASSERT(result->data != NULL);

        /* Transform each element */
        for (int i = 0; i < arr->length; i++) {
            void* src_ptr = (char*)arr->data + (i * arr->element_size);
            void* dst_ptr = (char*)result->data + (i * arr->element_size);
            mapper(src_ptr, dst_ptr, context);
        }
    } else {
        result->data = NULL;
    }

    return result;
}

DA_DEF void da_reduce(da_array arr, const void* initial, void* result,
                      void (*reducer)(void* accumulator, const void* element, void* context), void* context) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(initial != NULL);
    DA_ASSERT(result != NULL);
    DA_ASSERT(reducer != NULL);

    /* Initialize result with initial value */
    memcpy(result, initial, arr->element_size);

    /* Apply reducer to each element */
    for (int i = 0; i < arr->length; i++) {
        void* element_ptr = (char*)arr->data + (i * arr->element_size);
        reducer(result, element_ptr, context);
    }
}

DA_DEF int da_is_empty(da_array arr) {
    DA_ASSERT(arr != NULL);
    return arr->length == 0;
}

DA_DEF int da_find_index(da_array arr, int (*predicate)(const void* element, void* context), void* context) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(predicate != NULL);
    
    for (int i = 0; i < arr->length; i++) {
        void* element_ptr = (char*)arr->data + (i * arr->element_size);
        if (predicate(element_ptr, context)) {
            return i;
        }
    }
    
    return -1;  // Not found
}

DA_DEF int da_contains(da_array arr, int (*predicate)(const void* element, void* context), void* context) {
    return da_find_index(arr, predicate, context) != -1;
}

// Helper structure for qsort context
struct da_sort_context {
    int (*compare)(const void* a, const void* b, void* context);
    void* user_context;
};

// Global context for qsort (not thread-safe, but qsort isn't either)
static struct da_sort_context* da_sort_global_context = NULL;

// qsort comparison wrapper
static int da_sort_compare_wrapper(const void* a, const void* b) {
    struct da_sort_context* ctx = (struct da_sort_context*)da_sort_global_context;
    return ctx->compare(a, b, ctx->user_context);
}

DA_DEF void da_sort(da_array arr, int (*compare)(const void* a, const void* b, void* context), void* context) {
    DA_ASSERT(arr != NULL);
    DA_ASSERT(compare != NULL);
    
    if (arr->length <= 1) {
        return;  // Already sorted or empty
    }
    
    // Set up global context for qsort wrapper
    struct da_sort_context sort_ctx = { compare, context };
    da_sort_global_context = &sort_ctx;
    
    // Use standard library qsort
    qsort(arr->data, arr->length, arr->element_size, da_sort_compare_wrapper);
    
    // Clean up global context
    da_sort_global_context = NULL;
}

#endif /* DA_IMPLEMENTATION */

#endif /* DYNAMIC_ARRAY_H */
