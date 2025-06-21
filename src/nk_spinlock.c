#include "nk_spinlock.h"

/* Global Big Kernel Lock shared across all superlock instances. */
/* This spinlock composes all DAG/Lattice features of the base lock. */

nk_slock_t nk_spin_global = {0};

