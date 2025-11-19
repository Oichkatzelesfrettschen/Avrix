/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file kalloc.h
 * @brief Portable Kernel Heap Allocator
 *
 * Provides a simple bump-pointer allocator with free-list reuse for
 * embedded systems. Originally designed for ATmega328P (256B heap),
 * now configurable for different memory tiers.
 *
 * ## Design
 * - Fixed-size heap (configurable via NK_HEAP_SIZE)
 * - First-fit free-list allocation
 * - 2-byte alignment on 8/16-bit platforms, 4-byte on 32-bit
 * - Header overhead: 2-3 bytes per allocation
 * - No coalescing (keeps code simple, ~60 bytes flash)
 *
 * ## Memory Tiers
 * - Low-end (8-bit):   256 bytes  (ATmega328P)
 * - Mid-range (16-bit): 512-1024 bytes (ATmega1284, MSP430)
 * - High-end (32-bit):  2048-4096 bytes (ARM Cortex-M)
 *
 * ## Thread Safety
 * - NOT thread-safe by default (minimal overhead)
 * - Wrap kalloc/kfree in spinlocks for multi-threaded use
 * - Future: Add NK_KALLOC_THREAD_SAFE option for built-in locking
 *
 * ## Usage
 * ```c
 * kalloc_init();  // Initialize heap
 *
 * uint8_t *buf = kalloc(64);  // Allocate 64 bytes
 * if (!buf) {
 *     // Out of memory
 * }
 * // Use buf...
 * kfree(buf);  // Release memory
 * ```
 *
 * ## Limitations
 * - No malloc/free compatibility (uses uint8_t size, not size_t)
 * - No realloc support
 * - No coalescing (freed blocks remain fragmented)
 * - Maximum single allocation: 255 bytes
 */

#ifndef KERNEL_MM_KALLOC_H
#define KERNEL_MM_KALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "arch/common/hal.h"

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Heap size in bytes
 *
 * Default is tier-dependent:
 * - Low-end (8-bit):   256 bytes
 * - Mid-range (16-bit): 512 bytes
 * - High-end (32-bit):  2048 bytes
 *
 * Override via build system for custom configurations.
 */
#ifndef NK_HEAP_SIZE
#  if HAL_WORD_SIZE == 8
#    define NK_HEAP_SIZE 256u
#  elif HAL_WORD_SIZE == 16
#    define NK_HEAP_SIZE 512u
#  else
#    define NK_HEAP_SIZE 2048u
#  endif
#endif

/**
 * @brief Alignment requirement (bytes)
 *
 * - 8-bit platforms: 2-byte alignment
 * - 16-bit platforms: 2-byte alignment
 * - 32-bit platforms: 4-byte alignment
 */
#ifndef NK_KALLOC_ALIGN
#  if HAL_WORD_SIZE >= 32
#    define NK_KALLOC_ALIGN 4
#  else
#    define NK_KALLOC_ALIGN 2
#  endif
#endif

/**
 * @brief Enable thread-safe kalloc (future)
 *
 * When enabled, kalloc/kfree will use internal spinlocks.
 * Adds ~50 bytes flash + 1 byte RAM overhead.
 *
 * @note Not yet implemented. Use external locking for now.
 */
#ifndef NK_KALLOC_THREAD_SAFE
#  define NK_KALLOC_THREAD_SAFE 0
#endif

/* Compile-time validation */
_Static_assert(NK_HEAP_SIZE >= 64, "heap too small (min 64 bytes)");
_Static_assert(NK_HEAP_SIZE <= 65535u, "heap too large (max 64K)");
_Static_assert((NK_KALLOC_ALIGN & (NK_KALLOC_ALIGN - 1)) == 0,
               "alignment must be power of 2");

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize the kernel heap
 *
 * Resets the heap pointer and clears the free-list.
 * Must be called once at system startup before any kalloc/kfree calls.
 *
 * @note This function is NOT thread-safe. Call during single-threaded init.
 */
void kalloc_init(void);

/**
 * @brief Allocate memory from kernel heap
 *
 * Searches the free-list for a suitable block, or bumps the heap pointer
 * if no freed block is available.
 *
 * @param size Number of bytes to allocate (max 255)
 * @return Pointer to allocated memory, or NULL if out of memory
 *
 * @note Returned pointer is aligned to NK_KALLOC_ALIGN bytes.
 * @note Actual allocation may be larger due to alignment.
 * @note NOT thread-safe unless NK_KALLOC_THREAD_SAFE is enabled.
 */
void *kalloc(uint8_t size);

/**
 * @brief Free previously allocated memory
 *
 * Returns the block to the free-list for reuse. Does NOT coalesce
 * adjacent free blocks.
 *
 * @param ptr Pointer returned by kalloc(), or NULL (ignored)
 *
 * @note Freeing the same pointer twice is undefined behavior.
 * @note NOT thread-safe unless NK_KALLOC_THREAD_SAFE is enabled.
 */
void kfree(void *ptr);

/*═══════════════════════════════════════════════════════════════════
 * DEBUGGING & STATISTICS (optional)
 *═══════════════════════════════════════════════════════════════════*/

#if defined(NK_KALLOC_STATS) && NK_KALLOC_STATS

/**
 * @brief Heap statistics structure
 */
typedef struct {
    uint16_t total_size;      /**< Total heap size (bytes) */
    uint16_t used_bytes;      /**< Bytes currently allocated */
    uint16_t free_bytes;      /**< Bytes available (approx) */
    uint16_t peak_used;       /**< Peak allocation (bytes) */
    uint8_t  free_blocks;     /**< Number of free-list blocks */
    uint8_t  alloc_count;     /**< Total allocations (wraps at 255) */
    uint8_t  free_count;      /**< Total frees (wraps at 255) */
} kalloc_stats_t;

/**
 * @brief Get heap statistics
 *
 * @param[out] stats Pointer to statistics structure to fill
 *
 * @note Only available if NK_KALLOC_STATS is enabled.
 * @note Walking the free-list is O(n), use sparingly.
 */
void kalloc_get_stats(kalloc_stats_t *stats);

/**
 * @brief Reset peak usage counter
 */
void kalloc_reset_peak(void);

#endif /* NK_KALLOC_STATS */

/*═══════════════════════════════════════════════════════════════════
 * POSIX COMPATIBILITY (optional)
 *═══════════════════════════════════════════════════════════════════*/

#if defined(NK_KALLOC_POSIX_COMPAT) && NK_KALLOC_POSIX_COMPAT

/**
 * @brief POSIX malloc() compatibility wrapper
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL
 *
 * @note Limited to 255-byte allocations.
 * @note size_t is truncated to uint8_t.
 */
void *malloc(size_t size);

/**
 * @brief POSIX free() compatibility wrapper
 *
 * @param ptr Pointer to free
 */
void free(void *ptr);

/**
 * @brief POSIX calloc() compatibility wrapper
 *
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to zero-initialized memory, or NULL
 *
 * @note Limited to 255-byte total allocation.
 */
void *calloc(size_t nmemb, size_t size);

#endif /* NK_KALLOC_POSIX_COMPAT */

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_MM_KALLOC_H */
