#ifndef AVR_MEMGUARD_H
#define AVR_MEMGUARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file memguard.h
 * @brief Memory guard helpers for detecting buffer overflows.
 */

/** Pattern placed at the start and end of guarded regions. */
#define GUARD_PATTERN 0xA5U

/** Number of guard bytes on either side of a buffer. */
#define GUARD_BYTES   2U

/** Initialise guard bytes around a buffer. */
static inline void guard_init(uint8_t *mem, size_t size)
{
    for (size_t i = 0; i < GUARD_BYTES; ++i) {
        mem[i] = GUARD_PATTERN;
        mem[size - GUARD_BYTES + i] = GUARD_PATTERN;
    }
}

/** Validate guard bytes. Returns true if intact. */
static inline bool check_guard(const uint8_t *mem, size_t size)
{
    for (size_t i = 0; i < GUARD_BYTES; ++i) {
        if (mem[i] != GUARD_PATTERN || mem[size - GUARD_BYTES + i] != GUARD_PATTERN)
            return false;
    }
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* AVR_MEMGUARD_H */
