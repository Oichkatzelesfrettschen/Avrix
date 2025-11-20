/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file fork.c
 * @brief fork() stub implementation
 *
 * fork() is not supported on embedded systems without MMU.
 * This stub always returns -1 (error) with errno set to ENOSYS.
 *
 * POSIX Compliance: Stub only (low/mid/high profiles without MMU)
 */

#include "../posix_types.h"

/* Global errno (weak symbol, can be overridden) */
__attribute__((weak))
int errno = 0;

/**
 * @brief Create a child process (NOT SUPPORTED)
 *
 * fork() creates a new process by duplicating the calling process.
 * This requires an MMU and virtual memory, which most embedded
 * microcontrollers do not have.
 *
 * @return Always returns -1 (error)
 * @retval -1 Error, errno set to ENOSYS
 *
 * @note This is a stub function. It will always fail.
 * @note Some high-end systems with MPU may provide limited fork()
 *       functionality in the future, but it is not implemented yet.
 */
pid_t fork(void) {
    errno = ENOSYS;  /* Function not implemented */
    return -1;
}
