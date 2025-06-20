#ifndef AVR_KALLOC_H
#define AVR_KALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file kalloc.h
 * @brief Tiny kernel heap allocator with free-list reuse.
 */

/** Initialise the allocator and reset the heap pointer. */
void kalloc_init(void);

/** Allocate a block of memory from the 256 B heap. */
void *kalloc(uint8_t size);

/** Release a block previously obtained via kalloc. */
void kfree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* AVR_KALLOC_H */
