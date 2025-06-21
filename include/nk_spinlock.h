/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*────────────────────────── nk_spinlock.h ─────────────────────────────*
 * Thin wrapper around nk_superlock providing a unified spinlock
 * interface with encode/decode helpers.
 */
#pragma once
#include "nk_superlock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef nk_superlock_t       nk_spinlock_t;
typedef nk_superlock_capnp_t nk_spinlock_capnp_t;

#define NK_SPINLOCK_STATIC_INIT NK_SUPERLOCK_STATIC_INIT

static inline void nk_spinlock_init(nk_spinlock_t *s)
{
    nk_superlock_init(s);
}

static inline void nk_spinlock_lock(nk_spinlock_t *s)
{
    nk_superlock_lock(s, 0x1u);
}

static inline bool nk_spinlock_trylock(nk_spinlock_t *s)
{
    return nk_superlock_trylock(s, 0x1u);
}

static inline void nk_spinlock_unlock(nk_spinlock_t *s)
{
    nk_superlock_unlock(s);
}

static inline void nk_spinlock_lock_rt(nk_spinlock_t *s)
{
    nk_superlock_lock_rt(s, 0x1u);
}

static inline bool nk_spinlock_trylock_rt(nk_spinlock_t *s)
{
    return nk_superlock_trylock_rt(s, 0x1u);
}

static inline void nk_spinlock_unlock_rt(nk_spinlock_t *s)
{
    nk_superlock_unlock_rt(s);
}

static inline void nk_spinlock_encode(const nk_spinlock_t *s,
                                      nk_spinlock_capnp_t *out)
{
    nk_superlock_encode(s, out);
}

static inline void nk_spinlock_decode(nk_spinlock_t *s,
                                      const nk_spinlock_capnp_t *in)
{
    nk_superlock_decode(s, in);
}

static inline void nk_spinlock_matrix_set(nk_spinlock_t *s,
                                          unsigned idx, uint32_t val)
{
    nk_superlock_matrix_set(s, idx, val);
}

#define nk_spinlock_acquire(s)      nk_spinlock_lock((s))
#define nk_spinlock_release(s)      nk_spinlock_unlock((s))
#define nk_spinlock_acquire_rt(s)   nk_spinlock_lock_rt((s))
#define nk_spinlock_release_rt(s)   nk_spinlock_unlock_rt((s))

#ifdef __cplusplus
}
#endif

