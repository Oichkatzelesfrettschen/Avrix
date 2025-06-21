/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "nk_spinlock.h"

/* Global Big Kernel Lock shared across all spinlock instances. */
/* This spinlock composes all DAG/Lattice features of the base lock. */

nk_slock_t nk_bkl = {0};

