/*═══════════════════════════════════════════════════════════════════
 * nk_task.h - Nanokernel Task Scheduler for ATmega128
 *
 * Academic, comment-rich interface for a minimal preemptive scheduler
 * tuned for 32 kB flash and 2 kB SRAM targets. Pure C23 constructs are
 * used throughout to maximise safety without bloating code size.
 *═══════════════════════════════════════════════════════════════════*/

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*───────────────────────── Configuration ─────────────────────────*/
#ifndef NK_MAX_TASKS
#  define NK_MAX_TASKS      8        /* hard ceiling due to SRAM */
#endif
#ifndef NK_STACK_SIZE
#  define NK_STACK_SIZE     64       /* per-task stack bytes    */
#endif
#ifndef NK_QUANTUM_MS
#  define NK_QUANTUM_MS     10       /* scheduler slice in ms   */
#endif
#ifndef NK_OPT_DAG_WAIT
#  define NK_OPT_DAG_WAIT   1        /* dependency counters     */
#endif
#ifndef NK_OPT_STACK_GUARD
#  define NK_OPT_STACK_GUARD 1       /* sentinel-based checks   */
#endif

/*──────────────────────────── Types ─────────────────────────────*/

/* Task states – explicit underlying type keeps size predictable. */
typedef enum nk_state : uint8_t {
    NK_READY    = 0,
    NK_RUNNING  = 1,
    NK_BLOCKED  = 2,
    NK_SLEEPING = 3
} nk_state_t;

/* AVR stack pointer is a byte pointer. */
typedef uint8_t *nk_sp_t;

typedef void (*nk_task_fn)(void);

/* Task‑Control‑Block – fits within seven bytes. */
typedef struct nk_tcb {
    nk_sp_t   sp;          /* saved stack pointer                    */
    nk_state_t state:2;    /* packed state                           */
    uint8_t   priority:6;  /* 0..63                                  */
    uint8_t   pid;         /* task identifier                        */
    uint8_t   deps;        /* outstanding dependencies               */
    uint16_t  sleep_ticks; /* countdown for nk_sleep()               */
} nk_tcb_t;

static_assert(sizeof(nk_tcb_t) <= 8,
              "TCB too large for efficient access");

/*──────────────────────────── API ───────────────────────────────*/

void     nk_init(void);
bool     nk_task_create(nk_tcb_t *tcb, nk_task_fn entry,
                        uint8_t priority, void *stack_base,
                        size_t stack_size);
void     nk_start(void) [[noreturn]];
void     nk_yield(void);
void     nk_sleep(uint16_t ms);
uint8_t  nk_current_tid(void);
void     nk_switch_to(uint8_t tid);

#if NK_OPT_DAG_WAIT
void     nk_task_wait(uint8_t deps);
void     nk_task_signal(uint8_t tid);
#endif

/* Compatibility macros for legacy code. */
#define nk_sched_init()      nk_init()
#define nk_task_add(t,e,s,p,c) \
    ((void)(c), nk_task_create((t), (e), (p), (s), (NK_STACK_SIZE)))
#define nk_sched_run()       nk_start()
#define nk_cur_tid()         nk_current_tid()

#endif /* NK_TASK_H */
