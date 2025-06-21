/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef NK_SPINLOCK_H
#define NK_SPINLOCK_H

#include "nk_lock.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Simplified spinlock API built on top of the nk_slock primitives. */
typedef nk_slock_t nk_spinlock_t;

/** Static initializer suitable for global or static instances. */
#define NK_SPINLOCK_STATIC_INIT {0}

static inline void nk_spinlock_init(nk_spinlock_t *l)
{
    nk_slock_init(l);
}

static inline void nk_spinlock_lock_rt(nk_spinlock_t *l)
{
    nk_slock_lock(l);
}

static inline void nk_spinlock_unlock_rt(nk_spinlock_t *l)
{
    nk_slock_unlock(l);
}

#ifdef __cplusplus
}
#endif

#endif /* NK_SPINLOCK_H */
