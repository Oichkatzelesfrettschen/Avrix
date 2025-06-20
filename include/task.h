/*────────────────────────── nk_task.h ────────────────────────────
   µ-UNIX – task & scheduler interface            (ATmega328P, GCC-14)

   • 8-byte Task-Control-Block  (fits 8 TCBs → 64 B total)
   • Priority range 0-63  + 2-bit “class” channel for lock fairness
   • Optional DAG wait-counter for dependency scheduling
   • Pure C23 – no libc heap, no RTTI, no EH tables

   Public symbols implemented in nk_sched.c / isr.S
   -----------------------------------------------------------------*/
#ifndef NK_TASK_H
#define NK_TASK_H

/*————————————————————————— 1.  Includes & C-linkage ———————————————————*/
#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*————————————————————————— 2.  Compile-time knobs ————————————————*/

/** Maximum number of runnable tasks (incl. idle).  Hard SRAM ceiling. */
#ifndef NK_MAX_TASKS
#  define NK_MAX_TASKS  8
#endif

/** Enable DAG wait/signal bookkeeping (adds one byte per TCB). */
#ifndef NK_OPT_DAG_WAIT
#  define NK_OPT_DAG_WAIT  0
#endif

_Static_assert(NK_MAX_TASKS <= 8,
               "NK_MAX_TASKS>8 breaks SRAM/flash budgets!");

/*————————————————————————— 3.  Core types ————————————————————————*/

typedef uint16_t nk_sp_t;   /**< 16-bit stack pointer on classic AVR. */

/** Runnable state of a task (2 bits stored in the TCB). */
typedef enum nk_state {
    NK_READY   = 0,  /**< In run queue, waiting for its slice.   */
    NK_RUNNING = 1,  /**< Currently executing on the CPU.        */
    NK_BLOCKED = 2   /**< Sleeping on Door / lock / wait counter */
} nk_state_t;

/**
 * @struct nk_tcb
 * @brief **Task-Control-Block.**  *Exactly 8 bytes* when
 *        `NK_OPT_DAG_WAIT==0`.  Extra byte added otherwise.
 *
 * Memory image (little-endian):
 *
 * | Off | Bytes | Purpose
 * |-----|-------|------------------------------------------|
 * |  0  |  2    | `sp`  – saved stack pointer              |
 * |  2  |  1    | `state:2 | prio:6`                       |
 * |  3  |  1    | `pid`  – slot index 0…7                  |
 * |  4  |  1    | `class` – fairness channel (0…3)         |
 * |  5  |  1    | *pad*  or `deps` wait count              |
 * |  6  |  2    | reserved for future (keeps 8-byte align) |
 */
typedef struct nk_tcb {
    nk_sp_t    sp;
    uint8_t    state : 2;
    uint8_t    prio  : 6;
    uint8_t    pid;
    uint8_t    class : 2;
    uint8_t    _rsv  : 6;    /* unused bits stay zeroed          */
#if NK_OPT_DAG_WAIT
    uint8_t    deps;         /* outstanding dependency counter   */
#else
    uint8_t    _pad;
#endif
    uint16_t   _future;      /* keeps sizeof(TCB) == 8/10 aligned */
} nk_tcb_t;

#if NK_OPT_DAG_WAIT
_Static_assert(sizeof(nk_tcb_t) == 9 || sizeof(nk_tcb_t) == 10,
               "TCB size with DAG must pad to 10 bytes");
#else
_Static_assert(sizeof(nk_tcb_t) == 8, "TCB must stay 8 bytes");
#endif

/*————————————————————————— 4.  Public API ————————————————————————*/

/**
 * Initialise scheduler and idle task.
 *  – Configures Timer-0 for 1 kHz pre-emptive tick.
 *  – Sets up the initial run queue.
 */
void nk_sched_init(void);

/**
 * Add a new task to the run queue.
 *
 * @param tcb       Pointer to caller-allocated TCB
 * @param entry     Task entry function (never returns)
 * @param stack_top Pointer to **top** of caller-allocated stack
 * @param prio      Dynamic priority 0 (highest) … 63 (lowest)
 * @param class     0…3 fairness channel for lock arbitration
 */
void nk_task_add(nk_tcb_t *tcb,
                 void (*entry)(void),
                 void *stack_top,
                 uint8_t prio,
                 uint8_t class);

/** Start the round-robin scheduler – never returns. */
void nk_sched_run(void) __attribute__((noreturn));

/*——— Cooperative helpers (used by locks / Doors) ———————————————*/

/** Return the `pid` of the currently running task. */
uint8_t nk_cur_tid(void);

/** Yield CPU voluntarily (same slice keeps its remaining budget). */
void nk_yield(void);

/**
 * Low-level context switch (written in hand-tuned asm, isr.S).
 * Saves current regs → TCB[tid_old].sp, restores TCB[tid_new].sp,
 * then `reti`.
 */
void nk_switch_to(uint8_t tid);

/*——— Optional DAG wait / signal pair ———————————*/
#if NK_OPT_DAG_WAIT
/** Block `tcb` until its dependency counter becomes zero. */
void nk_task_block(nk_tcb_t *tcb, uint8_t deps);

/** Decrement dependency counter and unblock if it reaches zero. */
void nk_task_signal(nk_tcb_t *tcb);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NK_TASK_H */
