/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file nk_spinlock.h
 * @brief Unified spinlock primitive combining a global Big Kernel Lock (BKL)
 *        with optional fine-grained real-time bypass, built atop nk_slock.
 *
 * This implementation provides:
 *   - A global Big Kernel Lock (nk_bkl) for coarse-grained serialization.
 *   - Per-instance spinlocks for fine-grained locking.
 *   - Speculative copy-on-write (COW) state tracking via a small snapshot matrix.
 *   - Real-time bypass mode to skip global lock arbitration.
 *   - Cap'n Proto-compatible serialization helpers for lock state snapshots.
 *
 * All operations use atomic primitives from nk_slock and enforce
 * memory ordering for correctness in SMP contexts.
 */

#pragma once

#include "nk_lock.h"
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup spinlock
 *  @{
 */

/** @brief Underlying global Big Kernel Lock (BKL). */
extern nk_slock_t nk_bkl;

/**
 * @struct nk_spinlock_t
 * @brief Unified spinlock combining global and per-instance locking.
 */
typedef struct {
    nk_slock_t core;      /**< Underlying smart-lock primitive.     */
    uint8_t    dag_mask;  /**< Dependency bitmap for speculative ops. */
    uint8_t    rt_mode;   /**< Real-time mode flag: bypass global BKL. */
    uint32_t   matrix[4]; /**< Speculative COW state snapshot.      */
} nk_spinlock_t;

/** @brief Static initializer for nk_spinlock_t. */
#define NK_SPINLOCK_STATIC_INIT \
    { {0}, 0u, 0u, {0u, 0u, 0u, 0u} }

/**
 * @brief Initialize the global Big Kernel Lock.
 *
 * Must be called once during system startup before any spinlocks are used.
 */
static inline void nk_spinlock_global_init(void)
{
    nk_slock_init(&nk_bkl);
}

/**
 * @brief Initialize a spinlock instance.
 * @param[out] s Spinlock instance to initialize.
 */
static inline void nk_spinlock_init(nk_spinlock_t *s)
{
    assert(s != NULL);
    nk_slock_init(&s->core);
    s->dag_mask = 0u;
    s->rt_mode  = 0u;
    for (size_t i = 0; i < 4; ++i) {
        s->matrix[i] = 0u;
    }
}

/**
 * @brief Acquire the spinlock (global + core).
 * @param[in,out] s   Spinlock instance.
 * @param[in]     mask Dependency mask to record.
 */
static inline void nk_spinlock_lock(nk_spinlock_t *s, uint8_t mask)
{
    assert(s != NULL);
    nk_slock_lock(&nk_bkl);
    nk_slock_lock(&s->core);
    atomic_thread_fence(memory_order_acquire);
    s->dag_mask = mask;
    s->rt_mode  = 0u;
}

/**
 * @brief Try to acquire the spinlock without blocking.
 * @param[in,out] s   Spinlock instance.
 * @param[in]     mask Dependency mask to record.
 * @return true if lock was acquired; false otherwise.
 */
static inline bool nk_spinlock_trylock(nk_spinlock_t *s, uint8_t mask)
{
    assert(s != NULL);
    if (!nk_slock_trylock(&nk_bkl))
        return false;
    if (!nk_slock_trylock(&s->core)) {
        nk_slock_unlock(&nk_bkl);
        return false;
    }
    atomic_thread_fence(memory_order_acquire);
    s->dag_mask = mask;
    s->rt_mode  = 0u;
    return true;
}

/**
 * @brief Release the spinlock (core + global).
 * @param[in,out] s Spinlock instance.
 */
static inline void nk_spinlock_unlock(nk_spinlock_t *s)
{
    assert(s != NULL);
    atomic_thread_fence(memory_order_release);
    s->dag_mask = 0u;
    s->rt_mode  = 0u;
    nk_slock_unlock(&s->core);
    nk_slock_unlock(&nk_bkl);
}

/**
 * @brief Acquire the spinlock in real-time (bypass global BKL).
 * @param[in,out] s   Spinlock instance.
 * @param[in]     mask Dependency mask to record.
 */
static inline void nk_spinlock_lock_rt(nk_spinlock_t *s, uint8_t mask)
{
    assert(s != NULL);
    nk_slock_lock(&s->core);
    atomic_thread_fence(memory_order_acquire);
    s->dag_mask = mask;
    s->rt_mode  = 1u;
}

/**
 * @brief Try to acquire the spinlock in real-time mode.
 * @param[in,out] s   Spinlock instance.
 * @param[in]     mask Dependency mask to record.
 * @return true if lock was acquired; false otherwise.
 */
static inline bool nk_spinlock_trylock_rt(nk_spinlock_t *s, uint8_t mask)
{
    assert(s != NULL);
    if (!nk_slock_trylock(&s->core))
        return false;
    atomic_thread_fence(memory_order_acquire);
    s->dag_mask = mask;
    s->rt_mode  = 1u;
    return true;
}

/**
 * @brief Release the spinlock acquired in real-time mode.
 * @param[in,out] s Spinlock instance.
 */
static inline void nk_spinlock_unlock_rt(nk_spinlock_t *s)
{
    assert(s != NULL);
    atomic_thread_fence(memory_order_release);
    s->dag_mask = 0u;
    s->rt_mode  = 0u;
    nk_slock_unlock(&s->core);
}

/**
 * @struct nk_spinlock_capnp_t
 * @brief Cap'n Proto-friendly snapshot of spinlock state.
 */
typedef struct {
    uint8_t   dag_mask; /**< Dependency mask at snapshot time. */
    uint32_t  matrix[4];/**< Snapshot of speculative matrix state. */
} nk_spinlock_capnp_t;

/**
 * @brief Encode spinlock state into Cap'n Proto snapshot.
 * @param[in]  s   Spinlock instance.
 * @param[out] out Snapshot structure to populate.
 */
static inline void
nk_spinlock_encode(const nk_spinlock_t *s, nk_spinlock_capnp_t *out)
{
    assert(s != NULL && out != NULL);
    out->dag_mask = s->dag_mask;
    for (size_t i = 0; i < 4; ++i)
        out->matrix[i] = s->matrix[i];
}

/**
 * @brief Decode spinlock state from Cap'n Proto snapshot.
 * @param[out] s   Spinlock instance to update.
 * @param[in]  in  Snapshot structure to read.
 */
static inline void
nk_spinlock_decode(nk_spinlock_t *s, const nk_spinlock_capnp_t *in)
{
    assert(s != NULL && in != NULL);
    s->dag_mask = in->dag_mask;
    for (size_t i = 0; i < 4; ++i)
        s->matrix[i] = in->matrix[i];
}

/**
 * @brief Update a single entry in the speculative matrix.
 * @param[out] s   Spinlock instance.
 * @param[in]  idx Index [0..3] in the matrix.
 * @param[in]  val Value to set.
 */
static inline void
nk_spinlock_matrix_set(nk_spinlock_t *s, unsigned idx, uint32_t val)
{
    assert(s != NULL);
    assert(idx < 4);
    s->matrix[idx] = val;
}

/** Convenience aliases mirroring nk_slock API. */
#define nk_spinlock_acquire(s, m)    nk_spinlock_lock((s), (m))
#define nk_spinlock_release(s)       nk_spinlock_unlock((s))
#define nk_spinlock_acquire_rt(s, m) nk_spinlock_lock_rt((s), (m))
#define nk_spinlock_release_rt(s)    nk_spinlock_unlock_rt((s))

/** @} */

#ifdef __cplusplus
}
#endif
