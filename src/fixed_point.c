/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "fixed_point.h"

/**
 * Multiply two Q8.8 fixed-point values using only 8-bit operations.
 *
 * This function mirrors Algorithm 6.1 from the monograph. It performs a 16-bit
 * by 16-bit multiplication and keeps the middle 16 bits as the result, which
 * corresponds to shifting the 32-bit product right by eight.
 */
q8_8_t q8_8_mul(q8_8_t a, q8_8_t b) {
    /* Split operands into high and low bytes for manual 8-bit math. */
    uint8_t a_hi = (uint8_t)(a >> 8);
    uint8_t a_lo = (uint8_t)a;
    uint8_t b_hi = (uint8_t)(b >> 8);
    uint8_t b_lo = (uint8_t)b;

    /* 16-bit partial products. */
    uint16_t p0 = (uint16_t)a_lo * b_lo;       // LSB
    uint16_t p1 = (uint16_t)a_lo * b_hi;       // middle
    uint16_t p2 = (uint16_t)a_hi * b_lo;       // middle
    uint16_t p3 = (uint16_t)a_hi * b_hi;       // MSB

    /* Accumulate middle terms with carry handling. */
    uint16_t middle = p1 + p2 + (p0 >> 8);

    /* Compose the final 32-bit product. Only the middle 16 bits are needed */
    /* for Q8.8. Add rounding by examining bit 7 of the lower byte. */
    uint16_t result = (p3 << 8) | (middle & 0xFF);
    if (middle & 0x0100) {
        result += 0x0001; // propagate carry
    }
    if (p0 & 0x80) {
        result += 0x0001; // rounding
    }

    return (q8_8_t)result;
}
