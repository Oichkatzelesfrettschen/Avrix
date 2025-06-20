#ifndef AVR_SPINLOCK_H
#define AVR_SPINLOCK_H

#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file spinlock.h
 * @brief Quaternion spinlock implementation using GPIOR registers.
 */

/**
 * @brief Acquire a quaternion spinlock.
 *
 * @param lock_addr Offset of the IO register used as the lock byte. Must be
 *        in the range [GPIOR0, GPIOR2] for optimal latency.
 * @param mark      Identifier in range [1,3] representing the calling task.
 */
void spinlock_acquire(uint8_t lock_addr, uint8_t mark);

/**
 * @brief Release a quaternion spinlock.
 *
 * @param lock_addr Offset of the IO register used as the lock byte.
 */
void spinlock_release(uint8_t lock_addr);

#ifdef __cplusplus
}
#endif

#endif // AVR_SPINLOCK_H
