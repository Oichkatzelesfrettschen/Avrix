/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file pthread_once.c
 * @brief One-time initialization
 *
 * Implements pthread_once() for thread-safe one-time initialization.
 */

#include "pthread.h"
#include "arch/common/hal.h"

extern int errno;

/**
 * @brief Execute a function exactly once
 *
 * Ensures that init_routine is called exactly once, even if
 * pthread_once() is called concurrently from multiple threads.
 *
 * @param once_control Pointer to pthread_once_t control variable
 * @param init_routine Function to call exactly once
 * @return 0 on success, error number on failure
 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    if (!once_control || !init_routine) {
        return EINVAL;
    }

    /* Fast path: already initialized */
    if (once_control->done) {
        hal_memory_barrier();
        return 0;
    }

    /* Use compare-and-swap to ensure only one thread initializes */
    uint8_t expected = 0;
    if (hal_atomic_compare_exchange_u8(&once_control->done, &expected, 1)) {
        /* We won the race - call init_routine */
        init_routine();

        /* Ensure initialization is visible to all threads */
        hal_memory_barrier();
    } else {
        /* Another thread is initializing - spin until done */
        while (!once_control->done) {
            /* Busy-wait (could yield here) */
            hal_memory_barrier();
        }
    }

    return 0;
}
