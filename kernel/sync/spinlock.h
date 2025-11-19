/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file spinlock.h
 * @brief Portable Spinlock Primitives
 *
 * Provides hierarchical spinlock implementation using HAL atomic operations.
 * Originally designed for AVR, now portable across all architectures.
 *
 * ## Lock Types
 *
 * 1. **Fast Lock (flock)** - Simple TAS spinlock (1 byte)
 *    - Smallest footprint, no fairness guarantees
 *    - Good for short critical sections with low contention
 *
 * 2. **Quaternion Lock (qlock)** - Fair ticket lock (2 bytes, optional)
 *    - FIFO ordering prevents starvation
 *    - Slightly larger but provides fairness
 *    - Enable with NK_ENABLE_QLOCK=1
 *
 * 3. **Smart Lock (slock)** - Composable lock with optional features
 *    - Base: TAS spinlock
 *    - Optional: Beatty lattice fairness (compile-time)
 *    - Optional: DAG dependency tracking (compile-time)
 *    - Enable features with NK_ENABLE_LATTICE and NK_ENABLE_DAG
 *
 * 4. **Composite Spinlock** - High-level spinlock with BKL
 *    - Global Big Kernel Lock (BKL) for coarse-grained serialization
 *    - Per-instance locks for fine-grained control
 *    - Real-time mode to bypass BKL
 *    - Speculative COW snapshot support
 *
 * ## Usage
 *
 * ```c
 * // Simple fast lock
 * nk_flock_t lock = 0;
 * nk_flock_lock(&lock);
 * // critical section
 * nk_flock_unlock(&lock);
 *
 * // Smart lock with initialization
 * nk_slock_t slock;
 * nk_slock_init(&slock);
 * nk_slock_lock(&slock);
 * // critical section
 * nk_slock_unlock(&slock);
 *
 * // Composite spinlock (high-level)
 * nk_spinlock_t spin;
 * nk_spinlock_init(&spin);
 * nk_spinlock_lock(&spin, 0);  // mask = 0 (no dependencies)
 * // critical section
 * nk_spinlock_unlock(&spin);
 * ```
 */

#ifndef KERNEL_SYNC_SPINLOCK_H
#define KERNEL_SYNC_SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "arch/common/hal.h"

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Enable quaternion ticket lock (fair FIFO ordering)
 *
 * Adds ~50 bytes flash, 1 byte RAM per lock.
 * Provides starvation-free fairness.
 */
#ifndef NK_ENABLE_QLOCK
#  define NK_ENABLE_QLOCK 0
#endif

/**
 * @brief Enable Beatty lattice fairness for smart locks
 *
 * Uses golden ratio (φ) increment for starvation-free ticket assignment.
 * Adds ~100 bytes flash, 2-4 bytes RAM per lock.
 */
#ifndef NK_ENABLE_LATTICE
#  define NK_ENABLE_LATTICE 0
#endif

/**
 * @brief Enable DAG dependency tracking
 *
 * Allows locks to track up to 8 wait-for dependencies.
 * Adds ~50 bytes flash, 1 byte RAM per lock.
 */
#ifndef NK_ENABLE_DAG
#  define NK_ENABLE_DAG 0
#endif

/*═══════════════════════════════════════════════════════════════════
 * WORD SIZE DETECTION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Word size in bits (for ticket type selection)
 *
 * Determines the ticket counter type for lattice-based locks.
 * - 16-bit: uint16_t (AVR8)
 * - 32-bit: uint32_t (ARM Cortex-M, MSP430, host)
 */
#ifndef NK_WORD_BITS
#  if HAL_WORD_SIZE == 8
#    define NK_WORD_BITS 16  /* Use 16-bit tickets on 8-bit CPUs */
#  elif HAL_WORD_SIZE == 16
#    define NK_WORD_BITS 16
#  else
#    define NK_WORD_BITS 32
#  endif
#endif

/*═══════════════════════════════════════════════════════════════════
 * FAST LOCK (FLOCK) - Simple TAS Spinlock
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Fast lock type (1 byte)
 *
 * Simple test-and-set spinlock. No fairness guarantees.
 * Zero-initialized to unlocked state.
 */
typedef volatile uint8_t nk_flock_t;

/**
 * @brief Initialize a fast lock
 *
 * @param f Pointer to fast lock
 */
static inline void nk_flock_init(nk_flock_t *f) {
    *f = 0;
}

/**
 * @brief Try to acquire fast lock (non-blocking)
 *
 * @param f Pointer to fast lock
 * @return true if lock acquired, false if already held
 */
static inline bool nk_flock_try(nk_flock_t *f) {
    return !hal_atomic_test_and_set_u8(f);
}

/**
 * @brief Acquire fast lock (blocking)
 *
 * Spins until lock is acquired.
 *
 * @param f Pointer to fast lock
 */
static inline void nk_flock_lock(nk_flock_t *f) {
    while (!nk_flock_try(f)) {
        /* Busy-wait - could add hal_cpu_relax() here */
    }
}

/**
 * @brief Release fast lock
 *
 * @param f Pointer to fast lock
 */
static inline void nk_flock_unlock(nk_flock_t *f) {
    hal_atomic_exchange_u8(f, 0);
    hal_memory_barrier();
}

/*═══════════════════════════════════════════════════════════════════
 * QUATERNION TICKET LOCK (QLOCK) - Fair FIFO Spinlock
 *═══════════════════════════════════════════════════════════════════*/

#if NK_ENABLE_QLOCK

/**
 * @brief Quaternion ticket lock (2 bytes)
 *
 * Provides fair FIFO ordering using ticket numbers.
 * Prevents starvation.
 */
typedef struct {
    volatile uint8_t head;  /**< Current serving ticket */
    volatile uint8_t tail;  /**< Next ticket to issue */
} nk_qlock_t;

/**
 * @brief Initialize a quaternion lock
 *
 * @param q Pointer to quaternion lock
 */
static inline void nk_qlock_init(nk_qlock_t *q) {
    q->head = 0;
    q->tail = 0;
    hal_memory_barrier();
}

/**
 * @brief Acquire quaternion lock (blocking, fair)
 *
 * @param q Pointer to quaternion lock
 */
static inline void nk_qlock_lock(nk_qlock_t *q) {
    /* Atomically fetch and increment tail to get our ticket */
    uint8_t my_ticket = q->tail;
    hal_atomic_exchange_u8((volatile uint8_t *)&q->tail, my_ticket + 1);

    /* Wait until our ticket is being served */
    while (q->head != my_ticket) {
        hal_memory_barrier();
    }
    hal_memory_barrier();
}

/**
 * @brief Release quaternion lock
 *
 * @param q Pointer to quaternion lock
 */
static inline void nk_qlock_unlock(nk_qlock_t *q) {
    hal_memory_barrier();
    /* Increment head to serve next ticket */
    uint8_t next = q->head + 1;
    hal_atomic_exchange_u8((volatile uint8_t *)&q->head, next);
}

#endif /* NK_ENABLE_QLOCK */

/*═══════════════════════════════════════════════════════════════════
 * BEATTY LATTICE (TOURMALINE) - Starvation-Free Fairness
 *═══════════════════════════════════════════════════════════════════*/

#if NK_ENABLE_LATTICE

/**
 * @brief Ticket type (word-size dependent)
 */
#if NK_WORD_BITS == 32
    typedef uint32_t nk_ticket_t;
    #define NK_LATTICE_DELTA ((nk_ticket_t)(1657u * 1024u))  /* φ × 2^20 */
#else
    typedef uint16_t nk_ticket_t;
    #define NK_LATTICE_DELTA ((nk_ticket_t)1657u)            /* φ × 2^10 */
#endif

_Static_assert(NK_LATTICE_DELTA <= (nk_ticket_t)-1,
               "NK_LATTICE_DELTA exceeds ticket type range");

/**
 * @brief Global lattice ticket counter
 *
 * Monotonically increasing ticket using golden ratio increment.
 * Wraparound is harmless due to modular arithmetic.
 */
static volatile nk_ticket_t nk_lattice_ticket = 0;

/**
 * @brief Get next lattice ticket
 *
 * @return Next ticket number
 */
static inline nk_ticket_t nk_next_ticket(void) {
    nk_ticket_t ticket = nk_lattice_ticket + NK_LATTICE_DELTA;
    nk_lattice_ticket = ticket;
    hal_memory_barrier();
    return ticket;
}

#endif /* NK_ENABLE_LATTICE */

/*═══════════════════════════════════════════════════════════════════
 * SMART LOCK (SLOCK) - Composable Lock with Optional Features
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Smart lock structure
 *
 * Composable lock that can include optional features at compile time:
 * - Base: Fast lock (always included)
 * - Optional: Beatty lattice fairness
 * - Optional: DAG dependency tracking
 */
typedef struct {
    nk_flock_t base;              /**< Underlying fast lock */
#if NK_ENABLE_LATTICE
    volatile nk_ticket_t owner;   /**< Current ticket owner */
#endif
#if NK_ENABLE_DAG
    uint8_t dag_mask;             /**< DAG dependency mask (8 deps max) */
#endif
} nk_slock_t;

/**
 * @brief Static initializer for smart lock
 */
#define NK_SLOCK_STATIC_INIT {0}

/**
 * @brief Initialize a smart lock
 *
 * @param s Pointer to smart lock
 */
static inline void nk_slock_init(nk_slock_t *s) {
    nk_flock_init(&s->base);
#if NK_ENABLE_LATTICE
    s->owner = NK_LATTICE_DELTA;  /* First waiter wins immediately */
#endif
#if NK_ENABLE_DAG
    s->dag_mask = 0;
#endif
    hal_memory_barrier();
}

/**
 * @brief Acquire smart lock (blocking)
 *
 * @param s Pointer to smart lock
 */
static inline void nk_slock_lock(nk_slock_t *s) {
#if NK_ENABLE_LATTICE
    nk_ticket_t my = nk_next_ticket();
    for (;;) {
        nk_flock_lock(&s->base);
        if (s->owner == my) break;  /* My turn */
        nk_flock_unlock(&s->base);  /* Busy wait */
    }
#else
    nk_flock_lock(&s->base);
#endif
    hal_memory_barrier();
}

/**
 * @brief Try to acquire smart lock (non-blocking)
 *
 * @param s Pointer to smart lock
 * @return true if lock acquired, false otherwise
 */
static inline bool nk_slock_trylock(nk_slock_t *s) {
#if NK_ENABLE_LATTICE
    nk_ticket_t my = nk_next_ticket();
    if (!nk_flock_try(&s->base)) {
        return false;
    }
    if (s->owner != my) {
        nk_flock_unlock(&s->base);
        return false;
    }
#else
    if (!nk_flock_try(&s->base)) {
        return false;
    }
#endif
    hal_memory_barrier();
    return true;
}

/**
 * @brief Release smart lock
 *
 * @param s Pointer to smart lock
 */
static inline void nk_slock_unlock(nk_slock_t *s) {
    hal_memory_barrier();
#if NK_ENABLE_LATTICE
    s->owner += NK_LATTICE_DELTA;  /* Next ticket wins */
#endif
    nk_flock_unlock(&s->base);
}

/*═══════════════════════════════════════════════════════════════════
 * COMPOSITE SPINLOCK - High-Level Spinlock with BKL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Composite spinlock structure
 *
 * Combines global Big Kernel Lock (BKL) with per-instance locking.
 * Supports real-time mode to bypass BKL for low-latency critical sections.
 */
typedef struct {
    nk_slock_t core;       /**< Per-instance smart lock */
    uint8_t    dag_mask;   /**< Dependency bitmap for speculative ops */
    uint8_t    rt_mode;    /**< Real-time flag: bypass global BKL */
    uint32_t   matrix[4];  /**< Snapshot of speculative COW state */
} nk_spinlock_t;

/**
 * @brief Static initializer for composite spinlock
 */
#define NK_SPINLOCK_STATIC_INIT \
    { {0}, 0u, 0u, {0u, 0u, 0u, 0u} }

/**
 * @brief Global Big Kernel Lock (BKL)
 *
 * Coarse-grained global lock for serialization across all spinlocks.
 * Must be initialized before any spinlock operations.
 */
extern nk_slock_t nk_bkl;

/**
 * @brief Initialize the global Big Kernel Lock
 *
 * Must be called once during system initialization before any
 * spinlock operations.
 */
void nk_spinlock_global_init(void);

/**
 * @brief Initialize a composite spinlock
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_init(nk_spinlock_t *s);

/**
 * @brief Acquire spinlock (BKL + instance lock)
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 */
void nk_spinlock_lock(nk_spinlock_t *s, uint8_t mask);

/**
 * @brief Try to acquire spinlock (non-blocking)
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 * @return true if lock acquired, false otherwise
 */
bool nk_spinlock_trylock(nk_spinlock_t *s, uint8_t mask);

/**
 * @brief Release spinlock (instance lock + BKL)
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_unlock(nk_spinlock_t *s);

/**
 * @brief Acquire spinlock in real-time mode (bypass BKL)
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 */
void nk_spinlock_lock_rt(nk_spinlock_t *s, uint8_t mask);

/**
 * @brief Try to acquire spinlock in real-time mode
 *
 * @param s Pointer to spinlock
 * @param mask Dependency mask to record
 * @return true if lock acquired, false otherwise
 */
bool nk_spinlock_trylock_rt(nk_spinlock_t *s, uint8_t mask);

/**
 * @brief Release spinlock acquired in real-time mode
 *
 * @param s Pointer to spinlock
 */
void nk_spinlock_unlock_rt(nk_spinlock_t *s);

/*═══════════════════════════════════════════════════════════════════
 * COMPATIBILITY ALIASES
 *═══════════════════════════════════════════════════════════════════*/

/** @brief Alias for nk_flock_lock */
#define nk_flock_acq  nk_flock_lock

/** @brief Alias for nk_flock_unlock */
#define nk_flock_rel  nk_flock_unlock

/** @brief Alias for nk_slock_lock */
#define nk_slock_acq(l, node)  nk_slock_lock(l)

/** @brief Alias for nk_slock_unlock */
#define nk_slock_rel  nk_slock_unlock

/** @brief Alias for nk_spinlock_lock */
#define nk_spinlock_acquire(s, m)    nk_spinlock_lock((s), (m))

/** @brief Alias for nk_spinlock_unlock */
#define nk_spinlock_release(s)       nk_spinlock_unlock((s))

/** @brief Alias for nk_spinlock_lock_rt */
#define nk_spinlock_acquire_rt(s,m)  nk_spinlock_lock_rt((s),(m))

/** @brief Alias for nk_spinlock_unlock_rt */
#define nk_spinlock_release_rt(s)    nk_spinlock_unlock_rt((s))

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SYNC_SPINLOCK_H */
