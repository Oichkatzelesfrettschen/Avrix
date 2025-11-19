/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file exec.c
 * @brief exec*() family stub implementations
 *
 * exec() family functions are not supported on embedded systems
 * without dynamic loading capability.
 *
 * POSIX Compliance: Stub only (all profiles)
 */

#include "../posix_types.h"

extern int errno;

/**
 * @brief Execute a program (NOT SUPPORTED)
 *
 * @param path Pathname of executable
 * @param argv Argument array
 * @return Always returns -1 (error)
 */
int execv(const char *path, char *const argv[]) {
    (void)path;
    (void)argv;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Execute a program with environment (NOT SUPPORTED)
 *
 * @param path Pathname of executable
 * @param argv Argument array
 * @param envp Environment array
 * @return Always returns -1 (error)
 */
int execve(const char *path, char *const argv[], char *const envp[]) {
    (void)path;
    (void)argv;
    (void)envp;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Execute a program by filename (NOT SUPPORTED)
 *
 * @param file Filename of executable
 * @param argv Argument array
 * @return Always returns -1 (error)
 */
int execvp(const char *file, char *const argv[]) {
    (void)file;
    (void)argv;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Execute a program with variadic args (NOT SUPPORTED)
 *
 * @param path Pathname of executable
 * @param arg0 First argument (NULL-terminated list)
 * @return Always returns -1 (error)
 */
int execl(const char *path, const char *arg0, ...) {
    (void)path;
    (void)arg0;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Execute a program by filename with variadic args (NOT SUPPORTED)
 *
 * @param file Filename of executable
 * @param arg0 First argument (NULL-terminated list)
 * @return Always returns -1 (error)
 */
int execlp(const char *file, const char *arg0, ...) {
    (void)file;
    (void)arg0;
    errno = ENOSYS;
    return -1;
}
