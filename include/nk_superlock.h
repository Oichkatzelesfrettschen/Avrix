#pragma once
#include "nk_spinlock.h"
#define nk_superlock_t      nk_spinlock_t
#define NK_SUPERLOCK_STATIC_INIT  NK_SPINLOCK_STATIC_INIT

#define nk_superlock_init(l)            nk_spinlock_init(l)
#define nk_superlock_lock(l, m)         nk_spinlock_lock(l)
#define nk_superlock_trylock(l, m)      nk_spinlock_trylock(l)
#define nk_superlock_unlock(l)          nk_spinlock_unlock(l)
#define nk_superlock_lock_rt(l, m)      nk_spinlock_lock(l)
#define nk_superlock_trylock_rt(l, m)   nk_spinlock_trylock(l)
#define nk_superlock_unlock_rt(l)       nk_spinlock_unlock(l)
#define nk_superlock_acquire(l, m)      nk_superlock_lock((l), (m))
#define nk_superlock_release(l)         nk_superlock_unlock(l)
#define nk_superlock_acquire_rt(l, m)   nk_superlock_lock_rt((l), (m))
#define nk_superlock_release_rt(l)      nk_superlock_unlock_rt(l)

/* encoding / decoding become no-ops */
#define nk_superlock_encode(s, out)     ((void)(s), (void)(out))
#define nk_superlock_decode(s, in)      ((void)(s), (void)(in))
#define nk_superlock_matrix_set(s,i,v)  ((void)(s), (void)(i), (void)(v))

extern nk_spinlock_t nk_bkl;
