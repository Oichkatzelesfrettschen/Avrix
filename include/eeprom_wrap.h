/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef NK_EEPROM_WRAP_H
#define NK_EEPROM_WRAP_H

/**
 * @file eeprom_wrap.h
 * @brief Helpers for converting EEPROM offsets to pointers.
 */

#include <stdint.h>

/** Convert an EEPROM offset to a mutable byte pointer. */
static inline uint8_t *ee_ptr(uint16_t off)
{
    return (uint8_t *)(uintptr_t)off;
}

/** Convert an EEPROM offset to a const byte pointer. */
static inline const uint8_t *ee_cptr(uint16_t off)
{
    return (const uint8_t *)(uintptr_t)off;
}

#endif /* NK_EEPROM_WRAP_H */

