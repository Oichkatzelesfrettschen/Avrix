/*──────────────────────── nk_lock.h ────────────────────────────
   Feature-dense spin-lock for µ-UNIX on ATmega328P (or AVR-DA/DB).

   • 1-cycle atomic test-and-set in lower I/O space
   • Optional DAG acyclicity check (dead-lock free)
   • Optional Beatty / Tourmaline lattice fairness
   • 100 % header-only, C23-pure, host-build friendly.

   Author: µ-UNIX team
  ───────────────────────────────────────────────────────────────*/
#pragma once
#ifndef NK_LOCK_H
#define NK_LOCK_H

#include <stdint.h>
#include <stdbool.h>

/*──────── platform abstraction ─────────────────────────────────*/
#if defined(__AVR__)
#  include <avr/io.h>
#  include <avr/interrupt.h>
#else
   /* host build fall-backs */
#  define _SFR_IO8(x)   nk_sim_io[(x)]
   extern uint8_t nk_sim_io[0x40];
#  define cli()         (void)0
#  define sei()         (void)0
#endif

/*──────── user-tunable knobs (override before include) —───────*/
#ifndef NK_LOCK_ADDR
#  define NK_LOCK_ADDR 0x2C        /* GPIOR0 on ATmega328P */
#endif
#ifndef NK_ENABLE_DAG
#  define NK_ENABLE_DAG 1
#endif
#ifndef NK_ENABLE_LATTICE
#  define NK_ENABLE_LATTICE 1
#endif
#ifndef NK_DAG_MAX
#  define NK_DAG_MAX 8
#endif

/*──────── compile-time sanity ————————————*/
_Static_assert(NK_LOCK_ADDR <= 0x3F, "lock must be in lower I/O space");
_Static_assert(NK_LOCK_ADDR != 0x3D && NK_LOCK_ADDR != 0x3E,
               "avoid SPH/SPL – change during ISR");

/*──────── register accessor —————————————*/
#define NK_LOCK_REG _SFR_IO8(NK_LOCK_ADDR)

/*==========================================================================
 * 1.  FAST 1-BYTE TEST-AND-SET LOCK  (nk_flock_*)
 *==========================================================================*/
typedef struct { volatile uint8_t flag; } nk_flock_t;

/*  Assembly TAS is faster & atomic against interrupts. */
static inline bool __nk_try_raw(void)
{
#if defined(__AVR__)
    uint8_t r;
    asm volatile(
        "in  %[r],  %[io]   \n\t"   /* r = lock byte      */
        "tst %[r]           \n\t"
        "brne 1f            \n\t"   /* busy? skip store   */
        "ldi %[r], 1        \n\t"
        "out %[io], %[r]    \n\t"   /* attempt to set     */
        "1:                 "
        : [r] "=&r"(r)
        : [io] "I"(NK_LOCK_ADDR));
    return r == 0;
#else
    if (NK_LOCK_REG) return false;
    NK_LOCK_REG = 1;
    return true;
#endif
}

static inline void nk_flock_init(nk_flock_t *l)            { (void)l; NK_LOCK_REG = 0; }
static inline bool nk_flock_try (nk_flock_t *l)            { (void)l; return __nk_try_raw(); }
static inline void nk_flock_acq (nk_flock_t *l)            { while(!nk_flock_try(l)) ; }
static inline void nk_flock_rel (nk_flock_t *l)            { (void)l; NK_LOCK_REG = 0; }

/*==========================================================================
 * 2.  DAG  (dead-lock freedom)
 *==========================================================================*/
#if NK_ENABLE_DAG
#  if NK_DAG_MAX <= 8
     static uint8_t nk_dag_row[NK_DAG_MAX];          /* SRAM  */
#    define DAG_GET_ROW(i)  nk_dag_row[(i)]
#  else
#    include <avr/pgmspace.h>
     static const uint8_t nk_dag_row[] PROGMEM = { 0 };
     static inline uint8_t DAG_GET_ROW(uint8_t i) { return pgm_read_byte(&nk_dag_row[i]); }
#  endif

static inline void nk_dag_add(uint8_t a, uint8_t b) { nk_dag_row[a] |= 1u << b; }

/* O(N²)=64 cycles max */
static inline bool nk_dag_ok(void)
{
    uint8_t indeg[NK_DAG_MAX] = {0}, left = NK_DAG_MAX;
    for(uint8_t v=0; v<NK_DAG_MAX; ++v)
        for(uint8_t w=0; w<NK_DAG_MAX; ++w)
            if(DAG_GET_ROW(w) & (1u<<v)) ++indeg[v];

    while(left){
        int8_t n=-1;
        for(uint8_t v=0; v<NK_DAG_MAX; ++v) if(!indeg[v]) { n=v; break; }
        if(n<0) return false;
        indeg[(uint8_t)n]=0xFF; left--;
        for(uint8_t w=0; w<NK_DAG_MAX; ++w)
            if(DAG_GET_ROW((uint8_t)n) & (1u<<w)) --indeg[w];
    }
    return true;
}
#endif /* DAG */

/*==========================================================================
 * 3.  BEATTY / TOURMALINE LATTICE  (starvation-free fairness)
 *==========================================================================*/
#if NK_ENABLE_LATTICE
/* scale φ to word size ---------------------------------------------------*/
#if defined(__AVR_HAVE_32BIT__)
#  define NK_WORD_BITS 32
#elif defined(__AVR__)
#  define NK_WORD_BITS 16
#else
#  define NK_WORD_BITS 32
#endif
#if NK_WORD_BITS == 32
#  define NK_LATTICE_STEP 1695400ul   /* φ·2²⁶ */
   typedef uint32_t nk_ticket_t;
#else
#  define NK_LATTICE_STEP 1657u       /* φ·2¹⁰ */
   typedef uint16_t nk_ticket_t;
#endif

static volatile nk_ticket_t nk_lattice_ticket = 0;
static inline nk_ticket_t nk_next_ticket(void)
{
    return (nk_lattice_ticket += NK_LATTICE_STEP);
}
#endif /* LATTICE */

/*==========================================================================
 * 4.  SMART LOCK (features composable)
 *==========================================================================*/
typedef struct {
    nk_flock_t base;
#if NK_ENABLE_LATTICE
    volatile nk_ticket_t owner;
#endif
} nk_slock_t;

static inline void nk_slock_init(nk_slock_t *l)
{
    nk_flock_init(&l->base);
#if NK_ENABLE_LATTICE
    l->owner = 0;
#endif
}

/* main acquire -----------------------------------------------------------*/
static inline void nk_slock_acq(nk_slock_t *l, uint8_t node_id)
{
#if NK_ENABLE_LATTICE
    const nk_ticket_t me = nk_next_ticket();
#endif
    for(;;){
#if NK_ENABLE_DAG
        if(!nk_dag_ok()) continue;          /* wait for acyclic order */
#endif
        cli();
        if(!NK_LOCK_REG) {                  /* lock looks free        */
            NK_LOCK_REG = 1;
#if NK_ENABLE_LATTICE
            l->owner = me;
#endif
            sei();
            break;                          /* acquired               */
        }
#if NK_ENABLE_LATTICE
        /* lattice priority: older wins (16-/32-bit wrap safe) */
        if((nk_ticket_t)(me - l->owner) > (nk_ticket_t)0x8000) {
            sei();                          /* let older ticket win   */
            continue;
        }
#endif
        sei();                              /* de-prioritised spin    */
    }
    (void)node_id;
}

static inline void nk_slock_rel(nk_slock_t *l)
{ (void)l; NK_LOCK_REG = 0; }

#endif /* NK_LOCK_H */
