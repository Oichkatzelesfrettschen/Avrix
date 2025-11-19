/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file sleep.c
 * @brief sleep() and usleep() implementations
 *
 * Provides POSIX sleep functions using the kernel scheduler.
 */

#include "unistd.h"
#include "arch/common/hal.h"

/* Forward declaration of kernel sleep function */
extern void nk_sleep(uint16_t ms);

/**
 * @brief Sleep for specified number of seconds
 *
 * Suspends the calling thread for at least the specified number of
 * seconds. Uses the kernel scheduler's sleep functionality.
 *
 * @param seconds Number of seconds to sleep
 * @return 0 (cannot be interrupted on this implementation)
 *
 * @note Resolution is limited by system tick (typically 1 ms).
 * @note Sleeps for at least the requested time, may be longer.
 */
unsigned int sleep(unsigned int seconds) {
    if (seconds == 0) {
        return 0;
    }

    /* Convert seconds to milliseconds */
    /* Avoid overflow: sleep in 32-second chunks if needed */
    while (seconds > 32) {
        nk_sleep(32000);  /* 32 seconds */
        seconds -= 32;
    }

    /* Sleep remaining time */
    nk_sleep((uint16_t)(seconds * 1000));

    return 0;  /* Successfully slept (no signal interruption) */
}

/**
 * @brief Sleep for specified number of microseconds
 *
 * Suspends execution for the specified number of microseconds.
 *
 * @param usec Number of microseconds to sleep
 * @return 0 on success, -1 on error
 *
 * @note Precision depends on profile:
 *       - Low-end: May be stub (returns ENOSYS)
 *       - Mid/high-end: Millisecond precision via scheduler
 * @note For sub-millisecond delays, use hal_timer_delay_us() directly.
 */
int usleep(unsigned int usec) {
    if (usec == 0) {
        return 0;
    }

    /* Convert microseconds to milliseconds (round up) */
    uint16_t ms = (uint16_t)((usec + 999) / 1000);

    if (ms == 0) {
        /* Sub-millisecond delay: use HAL busy-wait */
        hal_timer_delay_us(usec);
    } else {
        /* Use scheduler sleep for millisecond delays */
        nk_sleep(ms);
    }

    return 0;
}
