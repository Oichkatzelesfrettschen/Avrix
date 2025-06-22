/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 *
 * @file src/nk_spinlock.c
 * @brief Definition and automatic initialization of the global Big Kernel Lock (BKL)
 *        for the nk_spinlock API.
 *
 * The Big Kernel Lock provides coarse-grained serialization across all
 * nk_spinlock instances. It is zero-initialized at load time and then
 * explicitly initialized via the module constructor to ensure correct
 * setup of internal state before use.
 */

#include "nk_spinlock.h"

/** @brief Global Big Kernel Lock shared by all spinlock instances. */
nk_slock_t nk_bkl = {0};

/**
 * @brief Module constructor to initialize the global Big Kernel Lock.
 *
 * This function runs before main()/kernel startup to ensure the BKL is ready
 * for use by any nk_spinlock. If your build environment does not support
 * constructor attributes, call nk_spinlock_global_init() manually during
 * system initialization.
 */
__attribute__((constructor))
static void nk_spinlock_module_init(void)
{
    nk_spinlock_global_init();
}
