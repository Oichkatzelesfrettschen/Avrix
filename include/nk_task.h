/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*────────────────────────── nk_task.h ────────────────────────────
   µ-UNIX – Task & scheduler interface (Arduino-Uno / ATmega328P)

   • 8 byte Task-Control-Block  → eight tasks fit in 64 B SRAM  
   • Priorities 0-63 + 2-bit “class” channel (lock fairness)  
   • Optional DAG wait-counter (dependency scheduler)  
   • Pure C23, freestanding: no libc heap / RTTI / EH tables

   Implementations:
       nk_sched.c  – core scheduler
       isr.S       – context-switch ASM & 1 kHz timer ISR
   ----------------------------------------------------------------*/
#ifndef NK_TASK_H
#define NK_TASK_H

/*──────────────── 1. Includes & C linkage ───────────────*/
#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {             /* header is C23-pure but callable from C++ */
#endif

/*──────────────── 2. Compile-time knobs ─────────────────*/

#ifndef NK_MAX_TASKS           /* incl. idle */
#  define NK_MAX_TASKS  8
#endif

#ifndef NK_OPT_DAG_WAIT        /* add 1 B per TCB */
#  define NK_OPT_DAG_WAIT  0
#endif

/* Per-task stack size in bytes and scheduler quantum in ms */
#ifndef NK_STACK_SIZE
#  define NK_STACK_SIZE 128u
#endif
#ifndef NK_QUANTUM_MS
#  define NK_QUANTUM_MS 10u
#endif

_Static_assert(NK_MAX_TASKS <= 8,
               "NK_MAX_TASKS > 8 breaks SRAM budget");

/*──────────────── 3. Core types ─────────────────────────*/

typedef uint16_t nk_sp_t;          /* AVR SP is 16 bit */

typedef enum nk_state : uint8_t {
    NK_READY   = 0,
    NK_RUNNING = 1,
    NK_BLOCKED = 2
} nk_state_t;

/* Task-Control-Block – must stay exactly 8 B unless DAG enabled */
typedef struct nk_tcb {
    nk_sp_t  sp;                   /* saved SP (little-endian)  */
    uint8_t  state : 2;
    uint8_t  prio  : 6;            /* dynamic priority 0-63     */
    uint8_t  pid;                  /* slot 0-(NK_MAX_TASKS-1)   */
    uint8_t  class : 2;            /* fairness channel 0-3      */
    uint8_t  _rsv  : 6;            /* keep zero – future use    */
#if NK_OPT_DAG_WAIT
    uint8_t  deps;                 /* outstanding deps counter  */
#else
    uint8_t  _pad;
#endif
    uint16_t _future;              /* keeps TCB aligned / 8 B   */
} nk_tcb_t;

#if NK_OPT_DAG_WAIT
_Static_assert(sizeof(nk_tcb_t) == 10,
               "TCB must pad to 10 bytes when DAG enabled");
#else
_Static_assert(sizeof(nk_tcb_t) == 8,
               "TCB must stay 8 bytes");
#endif

/*──────────────── 4. Public API ─────────────────────────*/

/** Initialise scheduler, idle task & 1 kHz tick. */
void scheduler_init(void);
#if defined(__GNUC__)
void nk_sched_init(void) __attribute__((alias("scheduler_init")));
#else
static inline void nk_sched_init(void) { scheduler_init(); }
#endif

/**
 * Add a task to the run queue.
 *
 * @param tcb       Caller-allocated TCB (zeroed)
 * @param entry     Task entry (never returns)
 * @param stack_top **Deprecated – ignored** (stacks are pre-allocated;
 *                  parameter will be removed in v0.2)
 * @param prio      0 (highest) … 63 (lowest)
 * @param class     0…3 fairness channel
 */
#if defined(__GNUC__)
__attribute__((deprecated("stack_top is ignored; will be removed")))
#endif
void nk_task_add(nk_tcb_t *tcb,
                 void (*entry)(void),
                 void *stack_top,
                 uint8_t prio,
                 uint8_t class);

/** Enter the scheduler – never returns. */
void scheduler_run(void) __attribute__((noreturn));
#if defined(__GNUC__)
void nk_sched_run(void) __attribute__((alias("scheduler_run"), noreturn));
#else
static inline void nk_sched_run(void) { scheduler_run(); }
#endif

/*─ Cooperative helpers — used by locks / Doors ──────────*/
uint8_t nk_cur_tid(void);     /**< current PID */
void    nk_yield(void);       /**< voluntary yield */

/* low-level switch (asm) */
void nk_switch_to(uint8_t tid);

/*─ Optional DAG wait / signal API ───────────────────────*/
#if NK_OPT_DAG_WAIT
void nk_task_block (nk_tcb_t *tcb, uint8_t deps);
void nk_task_signal(nk_tcb_t *tcb);
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif /* NK_TASK_H */
