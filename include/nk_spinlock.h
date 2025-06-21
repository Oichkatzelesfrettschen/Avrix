/*─────────────────────────── nk_spinlock.h ────────────────────────────*
 * Unified Big Kernel + fine grained spin-lock implementation.
 *
 *  - Built atop the ``nk_slock`` primitives
 *  - Maintains a small copy‑on‑write matrix for speculative state
 *  - Provides a global Big Kernel Lock (BKL) for coarse serialisation
 *  - Exposes a tiny Cap’n Proto compatible snapshot structure
 */
#pragma once
#include "nk_lock.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    nk_slock_t core;       /* base smart lock */
    uint8_t    dag_mask;   /* dependency bitmap               */
    uint8_t    rt_mode;    /* fine-grained real-time mode     */
    uint32_t   matrix[4];  /* COW matrix snapshot             */
} nk_spinlock_t;

#define NK_SPINLOCK_STATIC_INIT { {0}, 0u, 0u, {0, 0, 0, 0} }

extern nk_slock_t nk_spin_global;       /* global Big Kernel Lock */

static inline void nk_spinlock_init(nk_spinlock_t *s)
{
    nk_slock_init(&s->core);
    nk_slock_init(&nk_spin_global);
    s->dag_mask = 0u;
    s->rt_mode = 0u;
    for (unsigned i = 0; i < 4; ++i)
        s->matrix[i] = 0u;
}

static inline void nk_spinlock_lock(nk_spinlock_t *s, uint8_t mask)
{
    nk_slock_lock(&nk_spin_global);
    nk_slock_lock(&s->core);
    s->dag_mask = mask;
    s->rt_mode = 0u;
}

static inline bool nk_spinlock_trylock(nk_spinlock_t *s, uint8_t mask)
{
    if (!nk_flock_try(&nk_spin_global.base))
        return false;
    if (!nk_flock_try(&s->core.base)) {
        nk_flock_unlock(&nk_spin_global.base);
        return false;
    }
    s->dag_mask = mask;
    s->rt_mode = 0u;
    return true;
}

static inline void nk_spinlock_unlock(nk_spinlock_t *s)
{
    s->dag_mask = 0u;
    s->rt_mode = 0u;
    nk_slock_unlock(&s->core);
    nk_slock_unlock(&nk_spin_global);
}

/*--------------------------------------------------------------*
 * Real-time mode : bypass the global BKL for fine-grained locking
 *--------------------------------------------------------------*/
static inline void nk_spinlock_lock_rt(nk_spinlock_t *s, uint8_t mask)
{
    nk_slock_lock(&s->core);
    s->dag_mask = mask;
    s->rt_mode = 1u;
}

static inline bool nk_spinlock_trylock_rt(nk_spinlock_t *s, uint8_t mask)
{
    if (!nk_flock_try(&s->core.base))
        return false;
    s->dag_mask = mask;
    s->rt_mode = 1u;
    return true;
}

static inline void nk_spinlock_unlock_rt(nk_spinlock_t *s)
{
    s->dag_mask = 0u;
    s->rt_mode = 0u;
    nk_slock_unlock(&s->core);
}

/* Cap'n Proto like structure for serialised state */
typedef struct {
    uint8_t  dag_mask;
    uint32_t matrix[4];
} nk_spinlock_capnp_t;

static inline void nk_spinlock_encode(const nk_spinlock_t *s,
                                       nk_spinlock_capnp_t *out)
{
    out->dag_mask = s->dag_mask;
    for (unsigned i = 0; i < 4; ++i)
        out->matrix[i] = s->matrix[i];
}

static inline void nk_spinlock_decode(nk_spinlock_t *s,
                                       const nk_spinlock_capnp_t *in)
{
    s->dag_mask = in->dag_mask;
    for (unsigned i = 0; i < 4; ++i)
        s->matrix[i] = in->matrix[i];
}

/* granular matrix update for speculative COW state */
static inline void nk_spinlock_matrix_set(nk_spinlock_t *s,
                                           unsigned idx, uint32_t val)
{
    if (idx < 4)
        s->matrix[idx] = val;
}

/* shorthand aliases mirroring nk_slock API */
#define nk_spinlock_acquire(s, m)     nk_spinlock_lock((s), (m))
#define nk_spinlock_release(s)        nk_spinlock_unlock((s))
#define nk_spinlock_acquire_rt(s, m)  nk_spinlock_lock_rt((s), (m))
#define nk_spinlock_release_rt(s)     nk_spinlock_unlock_rt((s))

#ifdef __cplusplus
}
#endif

