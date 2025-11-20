/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file posix_types.h
 * @brief POSIX Standard Types for Embedded Systems
 *
 * Defines standard POSIX types used across the API. These types are
 * compatible with POSIX.1-2008 but sized appropriately for embedded
 * microcontrollers.
 *
 * Type sizing depends on architecture:
 * - 8-bit (AVR): Small types (pid_t = uint8_t)
 * - 16-bit (MSP430): Medium types (pid_t = uint16_t)
 * - 32-bit (ARM): Full types (pid_t = int32_t)
 */

#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════════
 * ARCHITECTURE-SPECIFIC SIZING
 *═══════════════════════════════════════════════════════════════════*/

/* Detect word size from HAL */
#include "arch/common/hal.h"

#if HAL_WORD_SIZE == 8
    /* 8-bit architecture (AVR) */
    #define POSIX_PID_T     uint8_t
    #define POSIX_UID_T     uint8_t
    #define POSIX_GID_T     uint8_t
    #define POSIX_OFF_T     uint16_t
    #define POSIX_SIZE_T    uint16_t
    #define POSIX_SSIZE_T   int16_t
    #define POSIX_TIME_T    uint32_t
    #define POSIX_CLOCK_T   uint32_t
#elif HAL_WORD_SIZE == 16
    /* 16-bit architecture (MSP430, PIC24) */
    #define POSIX_PID_T     uint16_t
    #define POSIX_UID_T     uint16_t
    #define POSIX_GID_T     uint16_t
    #define POSIX_OFF_T     uint32_t
    #define POSIX_SIZE_T    uint16_t
    #define POSIX_SSIZE_T   int16_t
    #define POSIX_TIME_T    uint32_t
    #define POSIX_CLOCK_T   uint32_t
#elif HAL_WORD_SIZE == 32
    /* 32-bit architecture (ARM Cortex-M) */
    #define POSIX_PID_T     int32_t
    #define POSIX_UID_T     uint32_t
    #define POSIX_GID_T     uint32_t
    #define POSIX_OFF_T     int32_t
    #define POSIX_SIZE_T    uint32_t
    #define POSIX_SSIZE_T   int32_t
    #define POSIX_TIME_T    int32_t
    #define POSIX_CLOCK_T   uint32_t
#else
    #error "Unsupported word size"
#endif

/*═══════════════════════════════════════════════════════════════════
 * POSIX STANDARD TYPES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Process ID type
 *
 * In embedded systems without MMU, this typically maps to task ID.
 */
typedef POSIX_PID_T pid_t;

/**
 * @brief User ID type (stub - not used in most embedded systems)
 */
typedef POSIX_UID_T uid_t;

/**
 * @brief Group ID type (stub - not used in most embedded systems)
 */
typedef POSIX_GID_T gid_t;

/**
 * @brief File offset type
 */
typedef POSIX_OFF_T off_t;

/**
 * @brief Size type (already defined as size_t in stddef.h)
 */
#ifndef _SIZE_T_DEFINED
typedef POSIX_SIZE_T size_t;
#define _SIZE_T_DEFINED
#endif

/**
 * @brief Signed size type
 */
typedef POSIX_SSIZE_T ssize_t;

/**
 * @brief Time type (seconds since epoch)
 */
typedef POSIX_TIME_T time_t;

/**
 * @brief Clock ticks type
 */
typedef POSIX_CLOCK_T clock_t;

/**
 * @brief Mode type (file permissions)
 */
typedef uint16_t mode_t;

/**
 * @brief Device ID type
 */
typedef uint16_t dev_t;

/**
 * @brief Inode number type
 */
typedef uint16_t ino_t;

/**
 * @brief Link count type
 */
typedef uint8_t nlink_t;

/**
 * @brief Block count type
 */
typedef uint16_t blkcnt_t;

/**
 * @brief Block size type
 */
typedef uint16_t blksize_t;

/*═══════════════════════════════════════════════════════════════════
 * PTHREAD TYPES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Thread identifier
 *
 * Maps to task ID in the scheduler.
 */
typedef pid_t pthread_t;

/**
 * @brief Thread attributes (opaque)
 */
typedef struct {
    uint8_t  detachstate;    /**< Detached or joinable */
    uint8_t  priority;       /**< Thread priority */
    size_t   stacksize;      /**< Stack size in bytes */
    void    *stackaddr;      /**< Stack address (if pre-allocated) */
} pthread_attr_t;

/**
 * @brief Mutex type (opaque)
 */
typedef struct {
    volatile uint8_t lock;   /**< Lock state (0=unlocked, 1=locked) */
    pid_t            owner;  /**< Owning thread ID */
    uint8_t          type;   /**< Mutex type (normal, recursive, etc.) */
} pthread_mutex_t;

/**
 * @brief Mutex attributes (opaque)
 */
typedef struct {
    uint8_t type;            /**< Mutex type */
    uint8_t protocol;        /**< Priority inheritance protocol */
} pthread_mutexattr_t;

/**
 * @brief Condition variable (opaque)
 */
typedef struct {
    volatile uint8_t waiters; /**< Number of waiting threads */
} pthread_cond_t;

/**
 * @brief Condition variable attributes (opaque)
 */
typedef struct {
    uint8_t dummy;           /**< Placeholder */
} pthread_condattr_t;

/**
 * @brief Once control (for pthread_once)
 */
typedef struct {
    volatile uint8_t done;   /**< 0 = not done, 1 = done */
} pthread_once_t;

/**
 * @brief Initializer for pthread_once_t
 */
#define PTHREAD_ONCE_INIT { 0 }

/**
 * @brief Thread-specific data key
 */
typedef uint8_t pthread_key_t;

/*═══════════════════════════════════════════════════════════════════
 * FILE DESCRIPTOR TYPES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief File descriptor type
 *
 * Small integer for embedded systems (0-255).
 */
typedef int8_t fd_t;

/*═══════════════════════════════════════════════════════════════════
 * ERROR CODES (errno values)
 *═══════════════════════════════════════════════════════════════════*/

#define EPERM           1   /**< Operation not permitted */
#define ENOENT          2   /**< No such file or directory */
#define ESRCH           3   /**< No such process */
#define EINTR           4   /**< Interrupted system call */
#define EIO             5   /**< I/O error */
#define ENXIO           6   /**< No such device or address */
#define E2BIG           7   /**< Argument list too long */
#define ENOEXEC         8   /**< Exec format error */
#define EBADF           9   /**< Bad file descriptor */
#define ECHILD          10  /**< No child processes */
#define EAGAIN          11  /**< Try again */
#define ENOMEM          12  /**< Out of memory */
#define EACCES          13  /**< Permission denied */
#define EFAULT          14  /**< Bad address */
#define ENOTBLK         15  /**< Block device required */
#define EBUSY           16  /**< Device or resource busy */
#define EEXIST          17  /**< File exists */
#define EXDEV           18  /**< Cross-device link */
#define ENODEV          19  /**< No such device */
#define ENOTDIR         20  /**< Not a directory */
#define EISDIR          21  /**< Is a directory */
#define EINVAL          22  /**< Invalid argument */
#define ENFILE          23  /**< File table overflow */
#define EMFILE          24  /**< Too many open files */
#define ENOTTY          25  /**< Not a typewriter */
#define ETXTBSY         26  /**< Text file busy */
#define EFBIG           27  /**< File too large */
#define ENOSPC          28  /**< No space left on device */
#define ESPIPE          29  /**< Illegal seek */
#define EROFS           30  /**< Read-only file system */
#define EMLINK          31  /**< Too many links */
#define EPIPE           32  /**< Broken pipe */
#define EDOM            33  /**< Math argument out of domain */
#define ERANGE          34  /**< Math result not representable */
#define EDEADLK         35  /**< Resource deadlock would occur */
#define ENAMETOOLONG    36  /**< File name too long */
#define ENOLCK          37  /**< No record locks available */
#define ENOSYS          38  /**< Function not implemented */
#define ENOTEMPTY       39  /**< Directory not empty */
#define ELOOP           40  /**< Too many symbolic links */
#define EWOULDBLOCK     EAGAIN  /**< Operation would block */
#define ENOMSG          42  /**< No message of desired type */
#define EIDRM           43  /**< Identifier removed */
#define ENOTSUP         95  /**< Not supported */
#define ETIMEDOUT       110 /**< Connection timed out */

/*═══════════════════════════════════════════════════════════════════
 * LIMITS (from limits.h)
 *═══════════════════════════════════════════════════════════════════*/

/* Open file limits (configurable per profile) */
#ifndef OPEN_MAX
#define OPEN_MAX        8   /**< Max open files per process */
#endif

/* Path length limits */
#ifndef PATH_MAX
#define PATH_MAX        256 /**< Max path length */
#endif

#ifndef NAME_MAX
#define NAME_MAX        64  /**< Max filename length */
#endif

/* Thread limits */
#ifndef PTHREAD_THREADS_MAX
#define PTHREAD_THREADS_MAX 16  /**< Max threads per process */
#endif

/*═══════════════════════════════════════════════════════════════════
 * NULL POINTER
 *═══════════════════════════════════════════════════════════════════*/

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* POSIX_TYPES_H */
