/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file unistd.h
 * @brief POSIX Standard Symbolic Constants and Types
 *
 * This header provides POSIX.1-2008 unistd.h functions for embedded
 * systems. Implementation level depends on profile:
 *
 * - Low-end (PSE51): Minimal (sleep, getpid, stubs)
 * - Mid-range (PSE52): Basic (above + usleep, I/O stubs)
 * - High-end (PSE54): Full (above + file I/O, process control)
 */

#ifndef POSIX_UNISTD_H
#define POSIX_UNISTD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../posix_types.h"

/*═══════════════════════════════════════════════════════════════════
 * STANDARD FILE DESCRIPTORS
 *═══════════════════════════════════════════════════════════════════*/

#define STDIN_FILENO    0   /**< Standard input */
#define STDOUT_FILENO   1   /**< Standard output */
#define STDERR_FILENO   2   /**< Standard error */

/*═══════════════════════════════════════════════════════════════════
 * PROCESS IDENTIFICATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Get process ID
 *
 * Returns the process ID of the calling process. In embedded systems
 * without true processes, this returns the task ID.
 *
 * @return Process/task ID
 *
 * @note Always succeeds.
 * @note Supported in all profiles.
 */
pid_t getpid(void);

/**
 * @brief Get parent process ID (STUB)
 *
 * Returns the parent process ID. In embedded systems, this is
 * typically not meaningful.
 *
 * @return Parent process ID (always 0 or 1)
 *
 * @note This is a stub function.
 */
pid_t getppid(void);

/**
 * @brief Get user ID (STUB)
 *
 * Returns the real user ID. Not supported on embedded systems.
 *
 * @return User ID (always 0)
 */
uid_t getuid(void);

/**
 * @brief Get effective user ID (STUB)
 *
 * Returns the effective user ID. Not supported on embedded systems.
 *
 * @return Effective user ID (always 0)
 */
uid_t geteuid(void);

/**
 * @brief Get group ID (STUB)
 *
 * Returns the real group ID. Not supported on embedded systems.
 *
 * @return Group ID (always 0)
 */
gid_t getgid(void);

/**
 * @brief Get effective group ID (STUB)
 *
 * Returns the effective group ID. Not supported on embedded systems.
 *
 * @return Effective group ID (always 0)
 */
gid_t getegid(void);

/*═══════════════════════════════════════════════════════════════════
 * PROCESS CONTROL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Create a child process (NOT SUPPORTED)
 *
 * See stubs/fork.c for implementation.
 *
 * @return -1 (error), errno = ENOSYS
 */
pid_t fork(void);

/**
 * @brief Execute a program (NOT SUPPORTED)
 *
 * See stubs/exec.c for implementation.
 */
int execv(const char *path, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
int execl(const char *path, const char *arg0, ...);
int execlp(const char *file, const char *arg0, ...);

/**
 * @brief Terminate the calling process
 *
 * Terminates the calling process with the specified exit status.
 * On embedded systems, this typically stops the current task.
 *
 * @param status Exit status (0 = success)
 *
 * @note Does not return.
 * @note Mid/high-end profiles: Task is marked as terminated.
 * @note Low-end profiles: May halt the system.
 */
void _exit(int status) __attribute__((noreturn));

/*═══════════════════════════════════════════════════════════════════
 * SLEEP & DELAY
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Sleep for a number of seconds
 *
 * Suspends execution of the calling thread for at least the specified
 * number of seconds.
 *
 * @param seconds Number of seconds to sleep
 * @return 0 on success, or number of seconds remaining if interrupted
 *
 * @note Supported in all profiles (uses scheduler).
 * @note Resolution is limited by system tick (typically 1 ms).
 * @note Cannot be interrupted by signals (no signal support).
 */
unsigned int sleep(unsigned int seconds);

/**
 * @brief Sleep for a number of microseconds
 *
 * Suspends execution for the specified number of microseconds.
 *
 * @param usec Number of microseconds to sleep
 * @return 0 on success, -1 on error
 *
 * @note Mid/high-end profiles: Uses scheduler with ms resolution.
 * @note Low-end profiles: May be a stub (returns ENOSYS).
 * @note Actual resolution depends on system tick frequency.
 */
int usleep(unsigned int usec);

/*═══════════════════════════════════════════════════════════════════
 * FILE I/O (stubs on low/mid, full on high)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Read from a file descriptor
 *
 * @param fd File descriptor
 * @param buf Buffer to read into
 * @param count Number of bytes to read
 * @return Number of bytes read, or -1 on error
 *
 * @note Low-end: Stub (returns ENOSYS)
 * @note Mid-range: Basic support (VFS required)
 * @note High-end: Full support
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 * @brief Write to a file descriptor
 *
 * @param fd File descriptor
 * @param buf Buffer to write from
 * @param count Number of bytes to write
 * @return Number of bytes written, or -1 on error
 *
 * @note Low-end: May redirect to UART console
 * @note Mid-range: Basic support (VFS required)
 * @note High-end: Full support
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 * @brief Close a file descriptor
 *
 * @param fd File descriptor
 * @return 0 on success, -1 on error
 */
int close(int fd);

/**
 * @brief Reposition read/write file offset
 *
 * @param fd File descriptor
 * @param offset Offset
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return New offset, or -1 on error
 */
off_t lseek(int fd, off_t offset, int whence);

/* Seek constants */
#define SEEK_SET 0  /**< Seek from beginning */
#define SEEK_CUR 1  /**< Seek from current position */
#define SEEK_END 2  /**< Seek from end */

/*═══════════════════════════════════════════════════════════════════
 * DIRECTORY OPERATIONS (stubs on low/mid, basic on high)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Change working directory (STUB)
 *
 * @param path Directory path
 * @return 0 on success, -1 on error
 */
int chdir(const char *path);

/**
 * @brief Get current working directory (STUB)
 *
 * @param buf Buffer to store path
 * @param size Buffer size
 * @return Pointer to buf on success, NULL on error
 */
char *getcwd(char *buf, size_t size);

/**
 * @brief Remove a directory (STUB)
 *
 * @param path Directory path
 * @return 0 on success, -1 on error
 */
int rmdir(const char *path);

/**
 * @brief Remove a file
 *
 * @param path File path
 * @return 0 on success, -1 on error
 */
int unlink(const char *path);

/*═══════════════════════════════════════════════════════════════════
 * ACCESS CONTROL (stubs)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Check file accessibility (STUB)
 *
 * @param path File path
 * @param mode Access mode (R_OK, W_OK, X_OK)
 * @return 0 if accessible, -1 otherwise
 */
int access(const char *path, int mode);

#define R_OK 4  /**< Test for read permission */
#define W_OK 2  /**< Test for write permission */
#define X_OK 1  /**< Test for execute permission */
#define F_OK 0  /**< Test for existence */

/*═══════════════════════════════════════════════════════════════════
 * SYNCHRONIZATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Synchronize file data with storage (STUB)
 *
 * @param fd File descriptor
 * @return 0 on success, -1 on error
 */
int fsync(int fd);

#ifdef __cplusplus
}
#endif

#endif /* POSIX_UNISTD_H */
