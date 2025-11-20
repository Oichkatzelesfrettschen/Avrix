/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file spinlock.c
 * @brief Composite Spinlock Implementation
 *
 * Implements high-level spinlock operations with global Big Kernel Lock (BKL).
 * Provides both normal and real-time (BKL-bypass) modes.
 */

#include "spinlock.h"
#include "arch/common/hal.h"
#include <stddef.h>

/*═══════════════════════════════════════════════════════════════════
 * GLOBAL BIG KERNEL LOCK (BKL)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Global Big Kernel Lock
 *
 * Provides coarse-grained serialization across all spinlock instances.
 * Must be initialized before any spinlock operations.
 */
nk_slock_t nk_bkl = {0};

/*═══════════════════════════════════════════════════════════════════
 * GLOBAL INITIALIZATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize the global Big Kernel Lock
 *
 * Must be called once during system startup before any spinlocks are used.
 * On some platforms, this may be called automatically via module constructors.
 * On others, it must be called explicitly from main() or kernel init.
 */
void nk_spinlock_global_init(void) {
    nk_slock_init(&nk_bkl);
}

/*═══════════════════════════════════════════════════════════════════
 * COMPOSITE SPINLOCK OPERATIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize a composite spinlock
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_init(nk_spinlock_t *s) {
    if (!s) return;

    nk_slock_init(&s->core);
    s->dag_mask = 0u;
    s->rt_mode  = 0u;
    for (size_t i = 0; i < 4; ++i) {
        s->matrix[i] = 0u;
    }
    hal_memory_barrier();
}

/**
 * @brief Acquire spinlock (global BKL + core lock)
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 */
void nk_spinlock_lock(nk_spinlock_t *s, uint8_t mask) {
    if (!s) return;

    /* Acquire global BKL first, then instance lock */
    nk_slock_lock(&nk_bkl);
    nk_slock_lock(&s->core);

    /* Memory barrier for acquire semantics */
    hal_memory_barrier();

    /* Record metadata */
    s->dag_mask = mask;
    s->rt_mode  = 0u;
}

/**
 * @brief Try to acquire spinlock without blocking
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 * @return true if acquired; false otherwise
 */
bool nk_spinlock_trylock(nk_spinlock_t *s, uint8_t mask) {
    if (!s) return false;

    /* Try to acquire global BKL */
    if (!nk_slock_trylock(&nk_bkl)) {
        return false;
    }

    /* Try to acquire instance lock */
    if (!nk_slock_trylock(&s->core)) {
        nk_slock_unlock(&nk_bkl);
        return false;
    }

    /* Memory barrier for acquire semantics */
    hal_memory_barrier();

    /* Record metadata */
    s->dag_mask = mask;
    s->rt_mode  = 0u;

    return true;
}

/**
 * @brief Release spinlock (core lock + global BKL)
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_unlock(nk_spinlock_t *s) {
    if (!s) return;

    /* Memory barrier for release semantics */
    hal_memory_barrier();

    /* Clear metadata */
    s->dag_mask = 0u;
    s->rt_mode  = 0u;

    /* Release locks in reverse order */
    nk_slock_unlock(&s->core);
    nk_slock_unlock(&nk_bkl);
}

/*═══════════════════════════════════════════════════════════════════
 * REAL-TIME MODE (BKL BYPASS)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Acquire spinlock in real-time mode (bypass global BKL)
 *
 * For low-latency critical sections that don't need global serialization.
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 */
void nk_spinlock_lock_rt(nk_spinlock_t *s, uint8_t mask) {
    if (!s) return;

    /* Only acquire instance lock, skip BKL */
    nk_slock_lock(&s->core);

    /* Memory barrier for acquire semantics */
    hal_memory_barrier();

    /* Record metadata */
    s->dag_mask = mask;
    s->rt_mode  = 1u;
}

/**
 * @brief Try to acquire spinlock in real-time mode
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 * @return true if acquired; false otherwise
 */
bool nk_spinlock_trylock_rt(nk_spinlock_t *s, uint8_t mask) {
    if (!s) return false;

    /* Try to acquire instance lock only */
    if (!nk_slock_trylock(&s->core)) {
        return false;
    }

    /* Memory barrier for acquire semantics */
    hal_memory_barrier();

    /* Record metadata */
    s->dag_mask = mask;
    s->rt_mode  = 1u;

    return true;
}

/**
 * @brief Release spinlock acquired in real-time mode
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_unlock_rt(nk_spinlock_t *s) {
    if (!s) return;

    /* Memory barrier for release semantics */
    hal_memory_barrier();

    /* Clear metadata */
    s->dag_mask = 0u;
    s->rt_mode  = 0u;

    /* Release instance lock only */
    nk_slock_unlock(&s->core);
}
