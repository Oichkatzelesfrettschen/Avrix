/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "nk_superlock.h"

/* Global Big Kernel Lock shared across all superlock instances. */
/* This spinlock composes all DAG/Lattice features of the base lock. */

/* Big Kernel Lock implemented as a basic spinlock. */
nk_spinlock_t nk_bkl = NK_SPINLOCK_STATIC_INIT;

