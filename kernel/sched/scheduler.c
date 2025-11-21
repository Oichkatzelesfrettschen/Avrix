/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file scheduler.c
 * @brief Portable Preemptive Scheduler for Embedded Systems
 */

#include "scheduler.h"
#include "arch/common/hal.h"
#include "avrix-config.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION (from profile/config)
 *═══════════════════════════════════════════════════════════════════*/

/* Use values from avrix-config.h, or defaults if missing */
#ifndef CONFIG_KERNEL_TASK_MAX
#  define CONFIG_KERNEL_TASK_MAX 8
#endif

#ifndef CONFIG_KERNEL_STACK_SIZE
#  define CONFIG_KERNEL_STACK_SIZE 128
#endif

#define NK_QUANTUM_MS 10
#define NK_OPT_STACK_GUARD CONFIG_KERNEL_PANIC_ON_FAULT

/*═══════════════════════════════════════════════════════════════════
 * SCHEDULER STATE
 *═══════════════════════════════════════════════════════════════════*/

#if defined(CONFIG_KERNEL_SCHED_TYPE_SINGLE)

/* ─── Single-Task Mode (Low Profile) ─── */
/* In this mode, the "scheduler" is just a loop for the main task.
   Only one task exists (main). */

void scheduler_init(void) {
    hal_timer_init(1000);
}

bool nk_task_create(nk_tcb_t *tcb, nk_task_fn entry, uint8_t prio, void *stack, size_t len) {
    (void)tcb; (void)prio; (void)stack; (void)len;
    /* In single mode, we only run the main entry point passed to run() or implicit main */
    entry();
    return true;
}

void scheduler_run(void) {
    /* Should be unreachable if task loops, but if it returns, we halt */
    for (;;) hal_idle();
}

void nk_yield(void) { /* No-op */ }
void nk_sleep(uint16_t ms) {
    /* Simple busy/idle wait */
    /* Note: In a real single-loop system, we'd use a timer flag */
    volatile uint16_t i = ms; /* Dummy implementation */
    while(i--) hal_idle();
}
uint8_t nk_current_tid(void) { return 0; }
void nk_task_exit(int status) { (void)status; for(;;) hal_idle(); }

/* IRQ handler does nothing for context switching */
void hal_timer_tick_handler(void) {}


#else /* CONFIG_KERNEL_SCHED_TYPE_PREEMPT or COOP */

/* ─── Multi-Task Scheduler (Mid/High Profile) ─── */

static struct {
    nk_tcb_t *tasks[CONFIG_KERNEL_TASK_MAX];
    uint8_t   count;
    uint8_t   current;
    volatile uint8_t quantum;
} nk_sched = {
    .count   = 0,
    .current = 0,
    .quantum = NK_QUANTUM_MS
};

/* Stack Management */
#if NK_OPT_STACK_GUARD
typedef struct {
    uint32_t guard_lo;
    uint8_t  data[CONFIG_KERNEL_STACK_SIZE];
    uint32_t guard_hi;
} nk_stack_t;
#define STACK_GUARD_PATTERN 0xDEADBEEF
static nk_stack_t nk_stacks[CONFIG_KERNEL_TASK_MAX] __attribute__((section(".noinit")));
#else
static uint8_t nk_stacks[CONFIG_KERNEL_TASK_MAX][CONFIG_KERNEL_STACK_SIZE] __attribute__((section(".noinit")));
#endif

static void update_sleep_timers(void) {
    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        nk_tcb_t *t = nk_sched.tasks[i];
        if (t->state == NK_SLEEPING && t->sleep_ticks) {
            if (--t->sleep_ticks == 0) {
                t->state = NK_READY;
            }
        }
    }
}

static uint8_t find_next_task(void) {
    uint8_t best  = nk_sched.current;
    uint8_t bestp = 0xFF;

    /* Round-robin search */
    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        uint8_t idx = (nk_sched.current + i + 1) % nk_sched.count;
        nk_tcb_t *t = nk_sched.tasks[idx];

        bool ready = (t->state == NK_READY);

        if (ready && t->priority < bestp) {
            best  = idx;
            bestp = t->priority;
        }
    }
    return best;
}

#if NK_OPT_STACK_GUARD
static void panic_stack_overflow(void) __attribute__((noreturn));
static void panic_stack_overflow(void) {
    hal_irq_disable();
    for (;;) hal_idle();
}
static inline void check_canaries(void) {
    nk_stack_t *stk = &nk_stacks[nk_sched.current];
    if (stk->guard_lo != STACK_GUARD_PATTERN ||
        stk->guard_hi != STACK_GUARD_PATTERN) {
        panic_stack_overflow();
    }
}
#endif

static void switch_to(uint8_t next) {
    if (next == nk_sched.current) return;

#if NK_OPT_STACK_GUARD
    check_canaries();
#endif

    nk_tcb_t *from = nk_sched.tasks[nk_sched.current];
    nk_tcb_t *to   = nk_sched.tasks[next];

    if (from->state == NK_RUNNING) from->state = NK_READY;
    to->state = NK_RUNNING;

    nk_sched.current = next;
    hal_context_switch((hal_context_t *)&from->sp, (hal_context_t *)&to->sp);
}

static inline void atomic_schedule(void) {
    hal_irq_disable();
    switch_to(find_next_task());
    hal_irq_enable();
}

void scheduler_init(void) {
#if NK_OPT_STACK_GUARD
    for (uint8_t i = 0; i < CONFIG_KERNEL_TASK_MAX; ++i) {
        nk_stacks[i].guard_lo = STACK_GUARD_PATTERN;
        nk_stacks[i].guard_hi = STACK_GUARD_PATTERN;
    }
#endif
    hal_timer_init(1000);
    nk_sched.count = 0;
    nk_sched.current = 0;
    nk_sched.quantum = NK_QUANTUM_MS;
}

bool nk_task_create(nk_tcb_t *tcb, nk_task_fn entry, uint8_t prio, void *stack, size_t stack_len) {
    if (!tcb || !entry) return false;
    if (nk_sched.count >= CONFIG_KERNEL_TASK_MAX) return false;

    if (!stack) {
#if NK_OPT_STACK_GUARD
        stack = nk_stacks[nk_sched.count].data;
        stack_len = CONFIG_KERNEL_STACK_SIZE;
#else
        stack = nk_stacks[nk_sched.count];
        stack_len = CONFIG_KERNEL_STACK_SIZE;
#endif
    }

    hal_context_init((hal_context_t *)&tcb->sp, entry, stack, stack_len);
    tcb->state = NK_READY;
    tcb->priority = (prio & 0x3F);
    tcb->pid = nk_sched.count;
    tcb->sleep_ticks = 0;

    hal_irq_disable();
    nk_sched.tasks[nk_sched.count++] = tcb;
    hal_irq_enable();
    return true;
}

void scheduler_run(void) {
    hal_irq_enable();
    switch_to(find_next_task());
    for (;;) hal_idle();
}

void nk_yield(void) {
    hal_irq_disable();
    nk_sched.quantum = 0;
    atomic_schedule();
}

void nk_sleep(uint16_t ms) {
    hal_irq_disable();
    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->state = NK_SLEEPING;
    t->sleep_ticks = ms;
    atomic_schedule();
}

uint8_t nk_current_tid(void) { return nk_sched.current; }

void nk_task_exit(int status) {
    (void)status;
    hal_irq_disable();
    nk_sched.tasks[nk_sched.current]->state = NK_TERMINATED;
    atomic_schedule();
    for (;;) hal_idle();
}

void hal_timer_tick_handler(void) {
    update_sleep_timers();
#ifdef CONFIG_KERNEL_SCHED_TYPE_PREEMPT
    if (--nk_sched.quantum == 0) {
        nk_sched.quantum = NK_QUANTUM_MS;
        uint8_t next = find_next_task();
        if (next != nk_sched.current) {
            switch_to(next);
        }
    }
#endif
}

#endif /* CONFIG_KERNEL_SCHED_TYPE... */

/* Aliases */
void nk_sched_init(void) __attribute__((alias("scheduler_init")));
void nk_init(void) __attribute__((alias("scheduler_init")));
void nk_sched_run(void) __attribute__((alias("scheduler_run")));
void nk_start(void) __attribute__((alias("scheduler_run")));
