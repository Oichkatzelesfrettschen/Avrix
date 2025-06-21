#pragma once
#include "nk_spinlock.h"

/* Backward compatibility wrapper */
#define nk_superlock_t              nk_spinlock_t
#define nk_superlock_capnp_t        nk_spinlock_capnp_t
#define NK_SUPERLOCK_STATIC_INIT    NK_SPINLOCK_STATIC_INIT
#define nk_bkl                      nk_spin_global
#define nk_superlock_init           nk_spinlock_init
#define nk_superlock_lock           nk_spinlock_lock
#define nk_superlock_trylock        nk_spinlock_trylock
#define nk_superlock_unlock         nk_spinlock_unlock
#define nk_superlock_lock_rt        nk_spinlock_lock_rt
#define nk_superlock_trylock_rt     nk_spinlock_trylock_rt
#define nk_superlock_unlock_rt      nk_spinlock_unlock_rt
#define nk_superlock_matrix_set     nk_spinlock_matrix_set
#define nk_superlock_encode         nk_spinlock_encode
#define nk_superlock_decode         nk_spinlock_decode
#define nk_superlock_acquire        nk_spinlock_acquire
#define nk_superlock_release        nk_spinlock_release
#define nk_superlock_acquire_rt     nk_spinlock_acquire_rt
#define nk_superlock_release_rt     nk_spinlock_release_rt
