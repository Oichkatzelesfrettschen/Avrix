/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file kalloc.c
 * @brief Portable Kernel Heap Allocator Implementation
 *
 * Simple bump-pointer allocator with free-list reuse.
 * Optimized for tiny embedded systems with limited SRAM.
 *
 * ## Algorithm
 * 1. kalloc(): Check free-list for suitable block (first-fit)
 * 2. If found, remove from list and return
 * 3. Otherwise, bump heap pointer and return new block
 * 4. kfree(): Prepend block to free-list (no coalescing)
 *
 * ## Memory Layout
 * ```
 * Heap:  [block_t][user data][block_t][user data]...
 *         ^        ^
 *         |        +-- Returned to user
 *         +-- Metadata (next ptr + size)
 * ```
 */

#include "kalloc.h"
#include <stddef.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * INTERNAL STRUCTURES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Free-list block header
 *
 * Prepended to each allocated block. When freed, the block is added
 * to the free-list via the `next` pointer.
 *
 * Size: 2 bytes (8-bit) or 3 bytes (16/32-bit with padding)
 */
typedef struct block {
    struct block *next;  /**< Next block in free-list (NULL if allocated) */
    uint8_t size;        /**< User-requested size (bytes) */
} block_t;

/*═══════════════════════════════════════════════════════════════════
 * ALIGNMENT HELPERS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Align size to NK_KALLOC_ALIGN boundary
 *
 * @param size Unaligned size
 * @return Aligned size
 */
static inline uint8_t align_size(uint8_t size) {
    uint8_t mask = NK_KALLOC_ALIGN - 1;
    return (size + mask) & ~mask;
}

/*═══════════════════════════════════════════════════════════════════
 * HEAP STATE
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Static heap buffer
 *
 * Fixed-size heap allocated in BSS section.
 */
static uint8_t heap[NK_HEAP_SIZE];

/**
 * @brief Current heap top pointer
 *
 * Points to the next free location in the heap for bump allocation.
 */
static uint8_t *heap_top;

/**
 * @brief Free-list head
 *
 * Singly-linked list of freed blocks available for reuse.
 */
static block_t *freelist;

#if NK_KALLOC_STATS
/**
 * @brief Heap statistics (optional)
 */
static kalloc_stats_t stats;
#endif

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API IMPLEMENTATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize the kernel heap
 *
 * Resets the heap pointer and clears the free-list.
 */
void kalloc_init(void) {
    heap_top = heap;
    freelist = NULL;

#if NK_KALLOC_STATS
    memset(&stats, 0, sizeof(stats));
    stats.total_size = NK_HEAP_SIZE;
    stats.free_bytes = NK_HEAP_SIZE;
#endif
}

/**
 * @brief Allocate memory from kernel heap
 *
 * @param size Number of bytes to allocate (max 255)
 * @return Pointer to allocated memory, or NULL if out of memory
 */
void *kalloc(uint8_t size) {
    if (size == 0) {
        return NULL;
    }

    /* Align size to platform requirements */
    size = align_size(size);

    /* Search free-list for suitable block (first-fit) */
    block_t **prev = &freelist;
    for (block_t *b = freelist; b != NULL; b = b->next) {
        if (b->size >= size) {
            /* Found suitable block - remove from free-list */
            *prev = b->next;

#if NK_KALLOC_STATS
            stats.alloc_count++;
            stats.free_blocks--;
#endif

            /* Return user data area (skip header) */
            return (void *)(b + 1);
        }
        prev = &b->next;
    }

    /* No suitable free block - allocate from heap */
    const size_t total_size = sizeof(block_t) + size;

    /* Check if enough space remains */
    if (heap_top + total_size > heap + NK_HEAP_SIZE) {
        return NULL;  /* Out of memory */
    }

    /* Allocate new block */
    block_t *blk = (block_t *)heap_top;
    blk->next = NULL;
    blk->size = size;
    heap_top += total_size;

#if NK_KALLOC_STATS
    stats.alloc_count++;
    stats.used_bytes += total_size;
    stats.free_bytes = NK_HEAP_SIZE - (heap_top - heap);
    if (stats.used_bytes > stats.peak_used) {
        stats.peak_used = stats.used_bytes;
    }
#endif

    /* Return user data area (skip header) */
    return (void *)(blk + 1);
}

/**
 * @brief Free previously allocated memory
 *
 * @param ptr Pointer returned by kalloc(), or NULL (ignored)
 */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }

    /* Get block header (immediately before user data) */
    block_t *blk = (block_t *)ptr - 1;

    /* Prepend to free-list (LIFO) */
    blk->next = freelist;
    freelist = blk;

#if NK_KALLOC_STATS
    stats.free_count++;
    stats.free_blocks++;
    /* Note: used_bytes not decremented (would require tracking allocations) */
#endif
}

/*═══════════════════════════════════════════════════════════════════
 * DEBUGGING & STATISTICS
 *═══════════════════════════════════════════════════════════════════*/

#if NK_KALLOC_STATS

/**
 * @brief Get heap statistics
 *
 * @param[out] out Pointer to statistics structure to fill
 */
void kalloc_get_stats(kalloc_stats_t *out) {
    if (!out) {
        return;
    }

    /* Copy current statistics */
    *out = stats;

    /* Count free-list blocks (O(n)) */
    uint8_t free_count = 0;
    for (block_t *b = freelist; b != NULL; b = b->next) {
        free_count++;
        if (free_count == 255) break;  /* Prevent overflow */
    }
    out->free_blocks = free_count;
}

/**
 * @brief Reset peak usage counter
 */
void kalloc_reset_peak(void) {
    stats.peak_used = stats.used_bytes;
}

#endif /* NK_KALLOC_STATS */

/*═══════════════════════════════════════════════════════════════════
 * POSIX COMPATIBILITY WRAPPERS
 *═══════════════════════════════════════════════════════════════════*/

#if NK_KALLOC_POSIX_COMPAT

/**
 * @brief POSIX malloc() compatibility wrapper
 */
void *malloc(size_t size) {
    if (size > 255) {
        return NULL;  /* Exceeds kalloc limit */
    }
    return kalloc((uint8_t)size);
}

/**
 * @brief POSIX free() compatibility wrapper
 */
void free(void *ptr) {
    kfree(ptr);
}

/**
 * @brief POSIX calloc() compatibility wrapper
 */
void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (total > 255) {
        return NULL;  /* Exceeds kalloc limit */
    }

    void *ptr = kalloc((uint8_t)total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

#endif /* NK_KALLOC_POSIX_COMPAT */
