/* SPDX-License-Identifier: MIT */
/* See LICENSE file in the repository root for full license information. */

/*─────────────────────────── nk_superlock.h ────────────────────────────*
 * Compatibility layer.  The former "superlock" API is now provided by
 * "nk_spinlock".  This header simply maps the old symbols to the new
 * implementation so existing code continues to build unmodified.
 *──────────────────────────────────────────────────────────────────────*/
#pragma once
#include "nk_spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef nk_spinlock_t       nk_superlock_t;
typedef nk_spinlock_capnp_t nk_superlock_capnp_t;

#define NK_SUPERLOCK_STATIC_INIT NK_SPINLOCK_STATIC_INIT

#define nk_superlock_init        nk_spinlock_init
#define nk_superlock_lock        nk_spinlock_lock
#define nk_superlock_trylock     nk_spinlock_trylock
#define nk_superlock_unlock      nk_spinlock_unlock
#define nk_superlock_lock_rt     nk_spinlock_lock_rt
#define nk_superlock_trylock_rt  nk_spinlock_trylock_rt
#define nk_superlock_unlock_rt   nk_spinlock_unlock_rt
#define nk_superlock_encode      nk_spinlock_encode
#define nk_superlock_decode      nk_spinlock_decode
#define nk_superlock_matrix_set  nk_spinlock_matrix_set
#define nk_superlock_acquire     nk_spinlock_acquire
#define nk_superlock_release     nk_spinlock_release
#define nk_superlock_acquire_rt  nk_spinlock_acquire_rt
#define nk_superlock_release_rt  nk_spinlock_release_rt

#ifdef __cplusplus
}
#endif
