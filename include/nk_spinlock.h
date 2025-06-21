/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*───────────────────────── nk_spinlock.h ──────────────────────────*
 * Thin wrapper around the nk_slock primitives providing a more
 * expressive spinlock interface with optional real-time semantics.
 * All operations are implemented inline for zero call overhead.
 *
 * The lock composes whatever features are enabled in nk_lock.h
 * (ticketing, DAG, Beatty lattice, ...).  No additional state is
 * introduced; nk_spinlock_t simply aliases nk_slock_t.
 *──────────────────────────────────────────────────────────────────*/
#pragma once
#include "nk_lock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* direct alias so the underlying implementation stays in one place */
typedef nk_slock_t nk_spinlock_t;

/* portable static initializer usable for globals */
#define NK_SPINLOCK_STATIC_INIT {0}

static inline void
nk_spinlock_init(nk_spinlock_t *s)
{
    nk_slock_init(s);
}

static inline void
nk_spinlock_lock_rt(nk_spinlock_t *s)
{
    /* Real-time lock maps to base slock semantics.  Platforms may
     * specialise this further if needed. */
    nk_slock_lock(s);
}

static inline void
nk_spinlock_unlock_rt(nk_spinlock_t *s)
{
    nk_slock_unlock(s);
}

#ifdef __cplusplus
}
#endif
