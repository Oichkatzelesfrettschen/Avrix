/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file pipe.c
 * @brief pipe() stub implementation
 *
 * pipe() creates a unidirectional data channel (pipe) that can be used
 * for inter-process communication.
 *
 * POSIX Compliance:
 * - Low-end: Stub (always fails)
 * - Mid-range: Stub (could be implemented with Door RPC)
 * - High-end: Full implementation (future)
 */

#include "../posix_types.h"

extern int errno;

/**
 * @brief Create a pipe (LIMITED SUPPORT)
 *
 * Creates a pipe, a unidirectional data channel that can be used
 * for IPC. On embedded systems, this is typically not needed as
 * tasks share the same address space.
 *
 * @param pipefd Array of two integers for read/write file descriptors
 * @return 0 on success, -1 on error
 * @retval -1 Error, errno set to ENOSYS (stub)
 *
 * @note This is a stub in low/mid-range profiles.
 * @note High-end profiles may implement this in the future.
 * @note Use Door RPC for efficient IPC on mid-range systems.
 */
int pipe(int pipefd[2]) {
    (void)pipefd;
    errno = ENOSYS;  /* Function not implemented */
    return -1;
}
