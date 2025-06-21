/*──────────────────────────────── nk_lock.h ──────────────────────────────
 * Spin-lock primitives for µ-UNIX on AVR.
 *
 *  • nk_flock   – 1-byte TAS
 *  • nk_qlock   – quaternion ticket   (fair)
 *  • nk_slock   – feature matrix: DAG + Beatty-lattice fairness
 *
 *  Word-size-aware Beatty step:
 *      16-bit   → 1657u          = φ × 2¹⁰
 *      32-bit   → 1657u × 1024u  = φ × 2²⁰   (avoids 32-bit wrap)
 *  Chosen at **compile-time**; no run-time penalty.
 *
 *  Portability: ``NK_LOCK_ADDR`` must reside in the lower I/O space
 *  (``≤ 0x3F``) for single-cycle access. 32‑bit AVR parts multiply
 *  ``NK_LATTICE_STEP`` by ``1024`` via ``NK_LATTICE_SCALE``.
 *─────────────────────────────────────────────────────────────────────────*/

#ifndef NK_LOCK_H
#define NK_LOCK_H

#include <stdint.h>
#include <stdbool.h>

/* Default lock byte maps to GPIOR0 for 1-cycle access.        */
#ifndef NK_LOCK_ADDR
#  define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "lock must be in lower I/O");

/* Feature toggles: default to disabled unless specified by build system */
#ifndef NK_ENABLE_QLOCK
#  define NK_ENABLE_QLOCK 0
#endif
#ifndef NK_ENABLE_LATTICE
#  define NK_ENABLE_LATTICE 0
#endif
#ifndef NK_ENABLE_DAG
#  define NK_ENABLE_DAG 0
#endif

/*--------------------------------------------------------------*
 * 0.  Build-time detection
 *--------------------------------------------------------------*/
/* Classic AVR ⇢ 16-bit “word”; AVR-XT (or host) ⇢ 32-bit word. */
#if defined(__AVR_HAVE_32BIT__)
#  define NK_WORD_BITS 32
#elif defined(__AVR__)
#  define NK_WORD_BITS 16
#else
#  define NK_WORD_BITS 32
#endif

/*--------------------------------------------------------------*
 * 1.  Low-level TAS flock   (8-bit, RAM-resident)
 *--------------------------------------------------------------*/
typedef volatile uint8_t nk_flock_t;

static inline void  nk_flock_init(nk_flock_t *f)           { *f = 0; }
static inline bool  nk_flock_try (nk_flock_t *f)           { return !__atomic_test_and_set(f, __ATOMIC_ACQUIRE); }
static inline void  nk_flock_lock(nk_flock_t *f)
{
    while (!nk_flock_try(f)) __asm__ __volatile__("nop");
}
static inline void  nk_flock_unlock(nk_flock_t *f)         { __atomic_clear(f, __ATOMIC_RELEASE); }

/*==============================================================*
 * 2.  Quaternion ticket lock  (fair, 1-byte state + 1-byte idx)
 *==============================================================*/
#if NK_ENABLE_QLOCK
typedef struct {
    volatile uint8_t head, tail;
} nk_qlock_t;

static inline void nk_qlock_init(nk_qlock_t *q)            { q->head = q->tail = 0; }
static inline void nk_qlock_lock(nk_qlock_t *q)
{
    uint8_t t = __atomic_fetch_add(&q->tail, 1, __ATOMIC_RELAXED);
    while (__atomic_load_n(&q->head, __ATOMIC_ACQUIRE) != t) __asm__("nop");
}
static inline void nk_qlock_unlock(nk_qlock_t *q)          { __atomic_fetch_add(&q->head, 1, __ATOMIC_RELEASE); }
#endif /* QLOCK */

/*==============================================================*
 * 3.  Beatty / Tourmaline lattice (starvation-free)
 *==============================================================*/
#if NK_ENABLE_LATTICE

#  define NK_LATTICE_STEP 1657u
#  if   NK_WORD_BITS == 32
#    define NK_LATTICE_SCALE 1024u
     typedef uint32_t        nk_ticket_t;
#  else
#    define NK_LATTICE_SCALE 1u
     typedef uint16_t        nk_ticket_t;
#  endif
#  define NK_LATTICE_DELTA (NK_LATTICE_STEP * NK_LATTICE_SCALE)

static volatile nk_ticket_t nk_lattice_ticket = 0;

/* Return *monotonically increasing* ticket; ℤₘ wrap is harmless */
static inline nk_ticket_t nk_next_ticket(void)
{
    return (nk_lattice_ticket += NK_LATTICE_DELTA);
}
#endif /* LATTICE */

/*==============================================================*
 * 4.  Smart-lock (slock) : compose features as needed
 *==============================================================*/
typedef struct {
    nk_flock_t base;
#   if NK_ENABLE_LATTICE
    volatile nk_ticket_t owner;
#   endif
#   if NK_ENABLE_DAG
    uint8_t dag_mask;                   /* up to 8 wait-for deps */
#   endif
} nk_slock_t;

static inline void nk_slock_init(nk_slock_t *s)
{
    nk_flock_init(&s->base);
#   if NK_ENABLE_LATTICE
    /* initial ticket so the first waiter wins immediately */
    s->owner = NK_LATTICE_DELTA;
#   endif
}

/* Acquire with optional lattice fairness (adds 4 cycles).      */
static inline void nk_slock_lock(nk_slock_t *s)
{
#   if NK_ENABLE_LATTICE
    nk_ticket_t my = nk_next_ticket();
    for (;;) {
        nk_flock_lock(&s->base);
        if (s->owner == my) break;          /* my turn            */
        nk_flock_unlock(&s->base);          /* busy wait          */
    }
#   else
    nk_flock_lock(&s->base);
#   endif
}

/*--------------------------------------------------------------*/
static inline void nk_slock_unlock(nk_slock_t *s)
{
#   if NK_ENABLE_LATTICE
    s->owner += NK_LATTICE_DELTA;            /* next ticket wins   */
#   endif
    nk_flock_unlock(&s->base);
}

/* Compatibility aliases --------------------------------------------------*/
#define nk_flock_acq  nk_flock_lock
#define nk_flock_rel  nk_flock_unlock
#define nk_slock_acq(l, node)  nk_slock_lock(l)
#define nk_slock_rel  nk_slock_unlock

#endif /* NK_LOCK_H */

