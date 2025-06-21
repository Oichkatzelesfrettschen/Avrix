#pragma once
#include "nk_lock.h"

/* Lightweight spinlock alias built atop nk_slock */
typedef nk_slock_t nk_spinlock_t;

/* Static initializer for global spinlocks */
#define NK_SPINLOCK_STATIC_INIT {0}

static inline void nk_spinlock_init(nk_spinlock_t *s)
{
    nk_slock_init(s);
}

static inline void nk_spinlock_lock(nk_spinlock_t *s)
{
    nk_slock_lock(s);
}

static inline bool nk_spinlock_trylock(nk_spinlock_t *s)
{
    return nk_flock_try(&s->base);
}

static inline void nk_spinlock_unlock(nk_spinlock_t *s)
{
    nk_slock_unlock(s);
}

/* Convenience aliases mirroring nk_slock API */
#define nk_spinlock_acquire(l)  nk_spinlock_lock(l)
#define nk_spinlock_release(l)  nk_spinlock_unlock(l)
