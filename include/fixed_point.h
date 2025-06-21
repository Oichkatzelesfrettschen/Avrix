/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef AVR_FIXED_POINT_H
#define AVR_FIXED_POINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file fixed_point.h
 * @brief Minimal fixed-point arithmetic for AVR (Q8.8).
 */

/**
 * Q8.8 fixed-point type. The upper byte holds integer part and the lower byte
 * holds fractional part. Operations are pure 8-bit arithmetic for the
 * ATmega328P.
 */
typedef int16_t q8_8_t;

/**
 * @brief Multiply two Q8.8 numbers.
 *
 * The implementation is cycle-accurate for the ATmega328P and relies solely on
 * 8-bit operations. It returns a Q8.8 result with rounding.
 *
 * @param a First operand.
 * @param b Second operand.
 * @return Result of a * b in Q8.8 format.
 */
q8_8_t q8_8_mul(q8_8_t a, q8_8_t b);

#ifdef __cplusplus
}
#endif

#endif // AVR_FIXED_POINT_H
