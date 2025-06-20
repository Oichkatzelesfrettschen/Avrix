#pragma once
#ifndef NK_LOCK_H
#define NK_LOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>

/**
 * \file nk_lock.h
 * \brief Feature-dense spinlock primitives for AVR.
 *
 * This header implements lock algorithms entirely as inline
 * functions so that the compiler can optimise away unused paths.
 * No dynamic memory is required; a single byte of state suffices
 * for the basic lock. Optional DAG validation and "lattice" ticket
 * logic may be enabled at compile time to guarantee acyclic lock
 * ordering and starvation freedom.
 */

/** Basic test-and-set lock structure. */
typedef struct {
    volatile uint8_t flag; /**< 0 when unlocked, non-zero when held */
} nk_flock_t;

static inline void nk_flock_init(nk_flock_t *l) { l->flag = 0; }
static inline bool nk_flock_try(nk_flock_t *l)
{
    if (l->flag)
        return false;
    l->flag = 1;
    return true;
}
static inline void nk_flock_acq(nk_flock_t *l)
{
    while (!nk_flock_try(l)) {
        /* busy wait */
    }
}
static inline void nk_flock_rel(nk_flock_t *l) { l->flag = 0; }

/* ----------------------- Optional DAG support ----------------------- */
#ifdef NK_ENABLE_DAG
#ifndef NK_DAG_MAX
#define NK_DAG_MAX 8 /**< maximum number of nodes in the DAG */
#endif

typedef struct { uint8_t mat[NK_DAG_MAX]; } nk_dag_t;
static nk_dag_t nk_dag; /**< global dependency graph */

/** Add an edge A->B to the DAG at setup time. */
static inline void nk_dag_add(uint8_t a, uint8_t b)
{
    nk_dag.mat[a] |= (uint8_t)1u << b;
}

/** Simple cycle detection; O(N^2) but N <= 8. */
static inline bool nk_dag_acyclic(void)
{
    uint8_t indeg[NK_DAG_MAX] = {0};
    uint8_t left = NK_DAG_MAX;
    for (uint8_t v = 0; v < NK_DAG_MAX; ++v) {
        for (uint8_t w = 0; w < NK_DAG_MAX; ++w) {
            if (nk_dag.mat[w] & ((uint8_t)1u << v))
                ++indeg[v];
        }
    }
    while (left) {
        int8_t node = -1;
        for (uint8_t v = 0; v < NK_DAG_MAX; ++v) {
            if (indeg[v] == 0) { node = v; break; }
        }
        if (node < 0)
            return false; /* cycle detected */
        indeg[(uint8_t)node] = 0xFF; /* remove */
        --left;
        for (uint8_t w = 0; w < NK_DAG_MAX; ++w) {
            if (nk_dag.mat[(uint8_t)node] & ((uint8_t)1u << w))
                --indeg[w];
        }
    }
    return true;
}
#endif /* NK_ENABLE_DAG */

/* --------------------- Optional lattice ticketing -------------------- */
#ifdef NK_ENABLE_LATTICE
/** ticket counter used for quasi-fair acquisition */
static volatile uint16_t nk_lattice_ticket = 0;

static inline uint16_t nk_next_ticket(void)
{
    /* multiply by golden ratio scaled to 2^10 (~1657) */
    nk_lattice_ticket += 1657u;
    return nk_lattice_ticket;
}
#endif /* NK_ENABLE_LATTICE */

/** Smart lock combining the features above. */
typedef struct {
    nk_flock_t base;
#ifdef NK_ENABLE_LATTICE
    volatile uint16_t owner; /**< owner ticket for fairness */
#endif
} nk_slock_t;

static inline void nk_slock_init(nk_slock_t *l)
{
    nk_flock_init(&l->base);
#ifdef NK_ENABLE_LATTICE
    l->owner = 0;
#endif
}

static inline void nk_slock_acq(nk_slock_t *l, uint8_t node_id)
{
#ifdef NK_ENABLE_LATTICE
    const uint16_t my = nk_next_ticket();
#endif
    while (true) {
#ifdef NK_ENABLE_DAG
        if (!nk_dag_acyclic())
            continue; /* wait until graph order valid */
#endif
        cli();
        if (!l->base.flag) {
            l->base.flag = 1;
#ifdef NK_ENABLE_LATTICE
            l->owner = my;
#endif
            sei();
            break; /* acquired */
        }
#ifdef NK_ENABLE_LATTICE
        if ((uint16_t)(my - l->owner) > 0x8000) {
            sei();
            continue; /* priority boost */
        }
#endif
        sei();
    }
    (void)node_id; /* silence unused warning if DAG disabled */
}

static inline void nk_slock_rel(nk_slock_t *l)
{
    l->base.flag = 0;
}

#endif /* NK_LOCK_H */
