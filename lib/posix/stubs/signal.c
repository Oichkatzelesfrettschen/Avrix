/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file signal.c
 * @brief signal() and kill() stub implementations
 *
 * Signal handling is not supported on most embedded systems due to
 * complexity and resource constraints.
 *
 * POSIX Compliance:
 * - Low-end: Stub (always fails)
 * - Mid-range: Stub (always fails)
 * - High-end: Basic implementation (future)
 */

#include "../posix_types.h"

extern int errno;

/* Signal numbers (from POSIX) */
#define SIGTERM  15  /**< Termination signal */
#define SIGKILL  9   /**< Kill signal */
#define SIGINT   2   /**< Interrupt signal */

/* Signal handler type */
typedef void (*sighandler_t)(int);

/**
 * @brief Set a signal handler (NOT SUPPORTED)
 *
 * @param signum Signal number
 * @param handler Signal handler function
 * @return Previous handler on success, SIG_ERR on error
 */
sighandler_t signal(int signum, sighandler_t handler) {
    (void)signum;
    (void)handler;
    errno = ENOSYS;
    return (sighandler_t)-1;  /* SIG_ERR */
}

/**
 * @brief Send a signal to a process (NOT SUPPORTED)
 *
 * @param pid Process ID to send signal to
 * @param sig Signal number
 * @return 0 on success, -1 on error
 */
int kill(pid_t pid, int sig) {
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}
