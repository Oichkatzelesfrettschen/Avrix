/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file pthread.h
 * @brief POSIX Threads API for Embedded Systems
 *
 * Provides POSIX.1-2008 pthread API for embedded microcontrollers.
 * Implementation wraps around the kernel scheduler.
 *
 * Profile Support:
 * - Low-end (PSE51): Stubs only (no threading)
 * - Mid-range (PSE52): Basic threading (create, join, mutex)
 * - High-end (PSE54): Full threading (above + condvar, barriers, etc.)
 */

#ifndef POSIX_PTHREAD_H
#define POSIX_PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../posix_types.h"

/*═══════════════════════════════════════════════════════════════════
 * PTHREAD CONSTANTS
 *═══════════════════════════════════════════════════════════════════*/

/* Detach state */
#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_CREATE_DETACHED     1

/* Mutex type */
#define PTHREAD_MUTEX_NORMAL        0
#define PTHREAD_MUTEX_RECURSIVE     1
#define PTHREAD_MUTEX_ERRORCHECK    2

/* Mutex protocol (priority inheritance) */
#define PTHREAD_PRIO_NONE           0
#define PTHREAD_PRIO_INHERIT        1
#define PTHREAD_PRIO_PROTECT        2

/* Cancellation */
#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

/* Return values */
#define PTHREAD_CANCELED ((void *)-1)

/*═══════════════════════════════════════════════════════════════════
 * STATIC INITIALIZERS
 *═══════════════════════════════════════════════════════════════════*/

#define PTHREAD_MUTEX_INITIALIZER { 0, 0, PTHREAD_MUTEX_NORMAL }
#define PTHREAD_COND_INITIALIZER  { 0 }

/*═══════════════════════════════════════════════════════════════════
 * THREAD MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Create a new thread
 *
 * Creates a new thread that executes the specified start routine.
 *
 * @param thread Pointer to pthread_t to store thread ID
 * @param attr Thread attributes (or NULL for defaults)
 * @param start_routine Function to execute in new thread
 * @param arg Argument to pass to start_routine
 * @return 0 on success, error number on failure
 *
 * @note Low-end: Returns ENOSYS (stub)
 * @note Mid/high-end: Creates new task via kernel scheduler
 * @note Stack size from attr or default (128-512 bytes)
 */
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg);

/**
 * @brief Wait for thread termination
 *
 * Suspends execution of the calling thread until the target thread
 * terminates.
 *
 * @param thread Thread to wait for
 * @param retval Pointer to store thread's return value (or NULL)
 * @return 0 on success, error number on failure
 *
 * @note Low-end: Returns ENOSYS (stub)
 * @note Mid/high-end: Blocks until thread exits
 */
int pthread_join(pthread_t thread, void **retval);

/**
 * @brief Detach a thread
 *
 * Marks the thread as detached. When a detached thread terminates,
 * its resources are automatically released.
 *
 * @param thread Thread to detach
 * @return 0 on success, error number on failure
 */
int pthread_detach(pthread_t thread);

/**
 * @brief Terminate calling thread
 *
 * Terminates the calling thread with the specified return value.
 *
 * @param retval Return value (available to pthread_join)
 *
 * @note Does not return.
 */
void pthread_exit(void *retval) __attribute__((noreturn));

/**
 * @brief Get calling thread's ID
 *
 * @return Thread ID of the calling thread
 */
pthread_t pthread_self(void);

/**
 * @brief Compare thread IDs
 *
 * @param t1 First thread ID
 * @param t2 Second thread ID
 * @return Non-zero if equal, 0 otherwise
 */
int pthread_equal(pthread_t t1, pthread_t t2);

/*═══════════════════════════════════════════════════════════════════
 * THREAD ATTRIBUTES
 *═══════════════════════════════════════════════════════════════════*/

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr);

/*═══════════════════════════════════════════════════════════════════
 * MUTEX (Mutual Exclusion)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize a mutex
 *
 * @param mutex Pointer to mutex to initialize
 * @param attr Mutex attributes (or NULL for defaults)
 * @return 0 on success, error number on failure
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

/**
 * @brief Destroy a mutex
 *
 * @param mutex Pointer to mutex to destroy
 * @return 0 on success, error number on failure
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 * @brief Lock a mutex
 *
 * Acquires the mutex, blocking if already locked.
 *
 * @param mutex Pointer to mutex to lock
 * @return 0 on success, error number on failure
 *
 * @note Low-end: May return ENOSYS (stub)
 * @note Mid/high-end: Blocks until mutex is available
 */
int pthread_mutex_lock(pthread_mutex_t *mutex);

/**
 * @brief Try to lock a mutex (non-blocking)
 *
 * Attempts to acquire the mutex without blocking.
 *
 * @param mutex Pointer to mutex to lock
 * @return 0 on success, EBUSY if already locked
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex);

/**
 * @brief Unlock a mutex
 *
 * Releases the mutex.
 *
 * @param mutex Pointer to mutex to unlock
 * @return 0 on success, error number on failure
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex);

/*═══════════════════════════════════════════════════════════════════
 * MUTEX ATTRIBUTES
 *═══════════════════════════════════════════════════════════════════*/

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol);

/*═══════════════════════════════════════════════════════════════════
 * CONDITION VARIABLES (high-end only)
 *═══════════════════════════════════════════════════════════════════*/

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

/*═══════════════════════════════════════════════════════════════════
 * ONCE CONTROL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Execute a function exactly once
 *
 * Ensures the init_routine is called exactly once, even if called
 * concurrently from multiple threads.
 *
 * @param once_control Pointer to pthread_once_t control variable
 * @param init_routine Function to call once
 * @return 0 on success, error number on failure
 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/*═══════════════════════════════════════════════════════════════════
 * THREAD-SPECIFIC DATA (limited support)
 *═══════════════════════════════════════════════════════════════════*/

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

/*═══════════════════════════════════════════════════════════════════
 * SCHEDULING (basic support)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Yield the processor to another thread
 *
 * Causes the calling thread to relinquish the CPU. The thread is
 * moved to the end of the queue for its priority.
 *
 * @return 0 on success
 */
int pthread_yield(void);
int sched_yield(void);  /* Alias */

#ifdef __cplusplus
}
#endif

#endif /* POSIX_PTHREAD_H */
