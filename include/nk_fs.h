/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef NK_FS_H
#define NK_FS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a 16-bit EEPROM byte address to a mutable pointer.
 *
 * These helpers wrap the integer-to-pointer cast required by the AVR
 * EEPROM API.  They are defined as ``static inline`` to avoid multiple
 * definition issues and impose zero call overhead.
 */
static inline uint8_t *ee_ptr(uint16_t addr)
{
    return (uint8_t *)(uintptr_t)addr; /* NOLINT(performance-no-int-to-ptr) */
}

/** Return a read-only EEPROM pointer. */
static inline const uint8_t *ee_cptr(uint16_t addr)
{
    return (const uint8_t *)(uintptr_t)addr; /* NOLINT(performance-no-int-to-ptr) */
}

/**
 * @file nk_fs.h
 * @brief TinyLog-4 EEPROM filesystem API.
 */

/** Initialise TinyLog-4 by scanning EEPROM for the last valid record. */
void nk_fs_init(void);

/** Append a key/value pair. Returns false on error or full store. */
bool nk_fs_put(uint16_t key, uint16_t val);

/** Retrieve the most recent value for a key. */
bool nk_fs_get(uint16_t key, uint16_t *val);

/** Mark a key as deleted. */
bool nk_fs_del(uint16_t key);

/** Optional garbage collection routine. */
void nk_fs_gc(void);

#ifdef __cplusplus
}
#endif

#endif /* NK_FS_H */
