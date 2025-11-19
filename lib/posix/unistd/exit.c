/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file exit.c
 * @brief Process termination
 *
 * Provides _exit() to terminate the calling task/process.
 */

#include "unistd.h"

/* Forward declaration - will be implemented when kernel is refactored */
extern void nk_task_exit(int status) __attribute__((noreturn));

/**
 * @brief Terminate the calling process
 *
 * Terminates the calling process/task immediately. On embedded systems,
 * this stops the current task and yields to the scheduler.
 *
 * @param status Exit status code (0 = success, non-zero = error)
 *
 * @note Does not return.
 * @note Does not call atexit() handlers (not supported).
 * @note Does not flush stdio buffers (minimal stdio).
 * @note Mid/high-end: Task is marked as terminated, scheduler continues.
 * @note Low-end: May halt the entire system if only one task exists.
 */
void _exit(int status) {
    /* Call kernel task exit function */
    nk_task_exit(status);

    /* Should never reach here */
    for (;;) {
        /* Infinite loop as fallback */
    }
}
