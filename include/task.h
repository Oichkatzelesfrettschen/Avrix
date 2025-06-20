/*──────────────────────────  nk_task.h  ──────────────────────────
   Minimal task-control interface for µ-UNIX on ATmega328P.
   Fits the 8-byte/TCB model already used in core.c / isr.S.

   Author: project µ-UNIX team   (C23, no heap, no libstdc++)
─────────────────────────────────────────────────────────────────*/
#ifndef NK_TASK_H
#define NK_TASK_H

#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ——— types ———————————————————————————————————————————————————— */

typedef uint16_t nk_sp_t;                 /* 8-bit core uses 16-bit SP  */

typedef enum {
    NK_READY   = 0,
    NK_RUNNING = 1,
    NK_BLOCKED = 2
} nk_state_t;

/* Task-Control Block – **8 bytes** when packed. */
typedef struct nk_tcb {
    nk_sp_t     sp;        /* saved SP (little-endian)            */
    nk_state_t  state:2;   /* bit-field – saves 6 bits/TCB        */
    uint8_t     prio:6;    /* 0…63  (lower); upper 2 bits = class */
    uint8_t     pid;       /* index 0…MAX_TASKS-1                 */
} nk_tcb_t;

#ifndef MAX_TASKS
#  define MAX_TASKS 8                     /* 8×8 B = 64 B SRAM          */
#endif
_Static_assert(MAX_TASKS <= 8,
               "MAX_TASKS > 8 breaks SRAM/stack budget!");

/* ——— scheduler API ———————————————————————————————————————— */

/* Called once from `main()` before tasks are added. */
void nk_sched_init(void);

/* Add a task; `stack_top` is **end+1** of the user-provided buffer. */
void nk_task_add(nk_tcb_t *t, void (*entry)(void), void *stack_top,
                 uint8_t priority /*0-63*/, uint8_t class   /*0-3*/);

/* Start the timer-driven scheduler; never returns. */
void nk_sched_run(void) __attribute__((noreturn));

/* Fast helpers needed by Doors / locks ---------------------------------- */

/* index of the task currently executing (exported from core.S) */
uint8_t nk_cur_tid(void);

/* Cooperative yield invoked by locks / doors (asm in isr.S). */
void     nk_yield(void);

/* Pre-emptive switch used by Doors: caller → callee, callee → caller. */
void     nk_switch_to(uint8_t tid);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NK_TASK_H */
