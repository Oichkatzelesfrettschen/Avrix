/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file getpid.c
 * @brief Process/task identification functions
 *
 * Provides POSIX process ID functions. On embedded systems without
 * true processes, these map to task IDs.
 */

#include "unistd.h"

/* Forward declaration of kernel function to get current task ID */
extern uint8_t nk_current_tid(void);

/**
 * @brief Get process ID
 *
 * Returns the process ID of the calling process. On embedded systems,
 * this returns the current task ID from the scheduler.
 *
 * @return Process/task ID (0 to max_tasks-1)
 *
 * @note Always succeeds.
 * @note On multi-threaded systems, this returns the thread ID.
 */
pid_t getpid(void) {
    return (pid_t)nk_current_tid();
}

/**
 * @brief Get parent process ID (STUB)
 *
 * Returns the parent process ID. On embedded systems without process
 * hierarchy, this returns a fixed value.
 *
 * @return Parent process ID (always 0 = kernel/idle)
 *
 * @note This is a stub. No parent/child relationship exists.
 */
pid_t getppid(void) {
    return 0;  /* Kernel/idle task is "parent" of all tasks */
}

/**
 * @brief Get user ID (STUB)
 *
 * Returns the real user ID. Embedded systems typically have no
 * user concept.
 *
 * @return User ID (always 0 = root)
 */
uid_t getuid(void) {
    return 0;
}

/**
 * @brief Get effective user ID (STUB)
 *
 * Returns the effective user ID.
 *
 * @return Effective user ID (always 0 = root)
 */
uid_t geteuid(void) {
    return 0;
}

/**
 * @brief Get group ID (STUB)
 *
 * Returns the real group ID.
 *
 * @return Group ID (always 0 = root group)
 */
gid_t getgid(void) {
    return 0;
}

/**
 * @brief Get effective group ID (STUB)
 *
 * Returns the effective group ID.
 *
 * @return Effective group ID (always 0 = root group)
 */
gid_t getegid(void) {
    return 0;
}
