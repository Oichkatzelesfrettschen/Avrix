/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*─────────────────────────── nk_superlock.h ────────────────────────────*
 * Legacy wrapper mapping the former "superlock" API onto the
 * new ``nk_spinlock`` implementation.
 */

#pragma once
#include "nk_spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type aliases ---------------------------------------------------------*/
typedef nk_spinlock_t        nk_superlock_t;
#define NK_SUPERLOCK_STATIC_INIT  NK_SPINLOCK_STATIC_INIT

typedef nk_spinlock_capnp_t  nk_superlock_capnp_t;

/* Function aliases -----------------------------------------------------*/
static inline void nk_superlock_init(nk_superlock_t *s)
{ nk_spinlock_init(s); }

static inline void nk_superlock_lock(nk_superlock_t *s, uint8_t m)
{ nk_spinlock_lock(s, m); }

static inline bool nk_superlock_trylock(nk_superlock_t *s, uint8_t m)
{ return nk_spinlock_trylock(s, m); }

static inline void nk_superlock_unlock(nk_superlock_t *s)
{ nk_spinlock_unlock(s); }

static inline void nk_superlock_lock_rt(nk_superlock_t *s, uint8_t m)
{ nk_spinlock_lock_rt(s, m); }

static inline bool nk_superlock_trylock_rt(nk_superlock_t *s, uint8_t m)
{ return nk_spinlock_trylock_rt(s, m); }

static inline void nk_superlock_unlock_rt(nk_superlock_t *s)
{ nk_spinlock_unlock_rt(s); }

static inline void nk_superlock_encode(const nk_superlock_t *s,
                                       nk_superlock_capnp_t *out)
{ nk_spinlock_encode(s, out); }

static inline void nk_superlock_decode(nk_superlock_t *s,
                                       const nk_superlock_capnp_t *in)
{ nk_spinlock_decode(s, in); }

static inline void nk_superlock_matrix_set(nk_superlock_t *s,
                                           unsigned idx, uint32_t val)
{ nk_spinlock_matrix_set(s, idx, val); }

/* Aliases mirroring previous macro names ------------------------------*/
#define nk_superlock_acquire(s, m)     nk_spinlock_acquire((s), (m))
#define nk_superlock_release(s)        nk_spinlock_release((s))
#define nk_superlock_acquire_rt(s, m)  nk_spinlock_acquire_rt((s), (m))
#define nk_superlock_release_rt(s)     nk_spinlock_release_rt((s))

#ifdef __cplusplus
}
#endif
