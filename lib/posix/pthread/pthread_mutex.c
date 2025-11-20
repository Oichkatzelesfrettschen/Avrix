/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file pthread_mutex.c
 * @brief Mutex (mutual exclusion) implementation
 *
 * Implements pthread_mutex_*() functions. Wraps kernel spinlock/mutex
 * primitives for portability.
 */

#include "pthread.h"
#include "arch/common/hal.h"

extern int errno;
extern uint8_t nk_current_tid(void);
extern void nk_yield(void);

/**
 * @brief Initialize a mutex
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    if (!mutex) {
        return EINVAL;
    }

    mutex->lock = 0;
    mutex->owner = 0;
    mutex->type = attr ? attr->type : PTHREAD_MUTEX_NORMAL;

    return 0;
}

/**
 * @brief Destroy a mutex
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (!mutex) {
        return EINVAL;
    }

    /* Check if locked */
    if (mutex->lock) {
        return EBUSY;
    }

    return 0;
}

/**
 * @brief Lock a mutex
 *
 * Uses HAL atomic operations for thread-safe locking.
 */
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!mutex) {
        return EINVAL;
    }

    pthread_t self = pthread_self();

    /* Recursive mutex: check if already owned */
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == self) {
        /* Already locked by this thread - just return success */
        /* (In full implementation, would increment recursion count) */
        return 0;
    }

    /* Error-check mutex: detect deadlock */
    if (mutex->type == PTHREAD_MUTEX_ERRORCHECK && mutex->owner == self) {
        return EDEADLK;
    }

    /* Spin until we acquire the lock */
    while (hal_atomic_test_and_set_u8(&mutex->lock)) {
        /* Lock is held, yield to other threads */
        nk_yield();
    }

    /* Lock acquired */
    mutex->owner = self;

    return 0;
}

/**
 * @brief Try to lock a mutex (non-blocking)
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if (!mutex) {
        return EINVAL;
    }

    pthread_t self = pthread_self();

    /* Recursive mutex: check if already owned */
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == self) {
        return 0;
    }

    /* Try to acquire the lock */
    if (hal_atomic_test_and_set_u8(&mutex->lock)) {
        return EBUSY;  /* Lock is held */
    }

    /* Lock acquired */
    mutex->owner = self;

    return 0;
}

/**
 * @brief Unlock a mutex
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!mutex) {
        return EINVAL;
    }

    pthread_t self = pthread_self();

    /* Error-check mutex: verify ownership */
    if (mutex->type == PTHREAD_MUTEX_ERRORCHECK && mutex->owner != self) {
        return EPERM;
    }

    /* Recursive mutex: (in full implementation, would decrement count) */
    /* For now, just release the lock */

    /* Release the lock */
    mutex->owner = 0;
    hal_atomic_exchange_u8(&mutex->lock, 0);

    return 0;
}

/*═══════════════════════════════════════════════════════════════════
 * MUTEX ATTRIBUTES
 *═══════════════════════════════════════════════════════════════════*/

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    if (!attr) return EINVAL;

    attr->type = PTHREAD_MUTEX_NORMAL;
    attr->protocol = PTHREAD_PRIO_NONE;

    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if (!attr) return EINVAL;
    if (type < 0 || type > PTHREAD_MUTEX_ERRORCHECK) return EINVAL;

    attr->type = (uint8_t)type;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
    if (!attr || !type) return EINVAL;

    *type = attr->type;
    return 0;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol) {
    if (!attr) return EINVAL;

    /* Priority inheritance not yet supported */
    if (protocol != PTHREAD_PRIO_NONE) {
        return ENOTSUP;
    }

    attr->protocol = (uint8_t)protocol;
    return 0;
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol) {
    if (!attr || !protocol) return EINVAL;

    *protocol = attr->protocol;
    return 0;
}
