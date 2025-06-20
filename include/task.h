/*────────────────────────── nk_task.h ───────────────────────────
   µ-UNIX task & scheduler interface  |  ATmega328P / GCC-14
   - 8-byte TCB (fits eight tasks → 64 B)
   - Priority 0-63  +  two-bit class for lock fairness
   - Optional DAG wait-count (compile-time enable)

   Author: µ-UNIX team – pure C23, no libc heap
  ───────────────────────────────────────────────────────────────*/
#ifndef NK_TASK_H
#define NK_TASK_H

#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*———— compile-time knobs ———————————————————————————————*/
#ifndef MAX_TASKS
#  define MAX_TASKS 8                 /* hard SRAM ceiling */
#endif
#ifndef NK_OPT_DAG_WAIT
#  define NK_OPT_DAG_WAIT 0           /* 1=include deps byte */
#endif
_Static_assert(MAX_TASKS <= 8,
               "MAX_TASKS>8 breaks SRAM/flash budgets!");

/*———— core types ————————————————————————————————————————*/
typedef uint16_t nk_sp_t;             /* 8-bit AVR has 16-bit SP */

typedef enum {
    NK_READY   = 0,
    NK_RUNNING = 1,
    NK_BLOCKED = 2
} nk_state_t;

/* 8-byte TCB (deps byte is optional but fits in padding) */
typedef struct nk_tcb {
    nk_sp_t     sp;                 /* saved stack pointer         */
    nk_state_t  state:2;
    uint8_t     prio:6;             /* lower 6 bits priority 0-63  */
    uint8_t     pid;                /* index in tcb pool           */
#if NK_OPT_DAG_WAIT
    uint8_t     deps;               /* outstanding dependencies    */
#else
    uint8_t     _pad;
#endif
} nk_tcb_t;
_Static_assert(sizeof(nk_tcb_t) == 8, "TCB must stay 8 bytes");

/*———— public API ————————————————————————————————————————*/
void     nk_sched_init(void);
void     nk_task_add(nk_tcb_t *t, void (*entry)(void), void *stack_top,
                     uint8_t priority, uint8_t class);
void     nk_sched_run(void) __attribute__((noreturn));

/* cooperative helpers (locks / doors) */
uint8_t  nk_cur_tid(void);
void     nk_yield(void);
void     nk_switch_to(uint8_t tid);   /* implemented in isr.S */

/* optional DAG wait / signal */
#if NK_OPT_DAG_WAIT
void     nk_task_block(nk_tcb_t *tcb, uint8_t deps);
void     nk_task_signal(nk_tcb_t *tcb);
#endif

#ifdef __cplusplus
}
#endif
#endif /* NK_TASK_H */
