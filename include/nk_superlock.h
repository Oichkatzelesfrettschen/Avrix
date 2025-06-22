/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 *
 * @file nk_superlock.h
 * @brief Backward-compatibility layer: maps the legacy “superlock” API
 *        to the unified nk_spinlock implementation.
 *
 * This header ensures existing code using nk_superlock_* symbols
 * continues to build unmodified by aliasing them to the new
 * nk_spinlock_* APIs.
 */

#pragma once

#include "nk_spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef nk_superlock_t
 * @brief Legacy alias for unified spinlock type.
 */
typedef nk_spinlock_t       nk_superlock_t;

/**
 * @typedef nk_superlock_capnp_t
 * @brief Legacy alias for Cap’n Proto snapshot of spinlock state.
 */
typedef nk_spinlock_capnp_t nk_superlock_capnp_t;

/** @brief Static initializer for nk_superlock_t (alias of NK_SPINLOCK_STATIC_INIT). */
#define NK_SUPERLOCK_STATIC_INIT NK_SPINLOCK_STATIC_INIT

/* Initialization and control operations (aliased to nk_spinlock API) */
#define nk_superlock_init        nk_spinlock_init
#define nk_superlock_lock        nk_spinlock_lock
#define nk_superlock_trylock     nk_spinlock_trylock
#define nk_superlock_unlock      nk_spinlock_unlock

/* Real-time (fine-grained bypass) operations */
#define nk_superlock_lock_rt     nk_spinlock_lock_rt
#define nk_superlock_trylock_rt  nk_spinlock_trylock_rt
#define nk_superlock_unlock_rt   nk_spinlock_unlock_rt

/* Serialization helpers */
#define nk_superlock_encode      nk_spinlock_encode
#define nk_superlock_decode      nk_spinlock_decode

/* Speculative matrix update */
#define nk_superlock_matrix_set  nk_spinlock_matrix_set

/* Convenience aliases */
#define nk_superlock_acquire     nk_spinlock_acquire
#define nk_superlock_release     nk_spinlock_release
#define nk_superlock_acquire_rt  nk_spinlock_acquire_rt
#define nk_superlock_release_rt  nk_spinlock_release_rt

#ifdef __cplusplus
}
#endif
