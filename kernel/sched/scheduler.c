/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file scheduler.c
 * @brief Portable Preemptive Scheduler for Embedded Systems
 *
 * This is a refactored version of the original nk_task.c from µ-UNIX,
 * made portable by using the HAL instead of AVR-specific code.
 *
 * Features:
 * - Priority-based round-robin scheduling
 * - 1 kHz system tick (configurable)
 * - Optional DAG dependency tracking
 * - Optional stack overflow detection
 * - Profile-configurable (low/mid/high-end)
 *
 * Flash: ≈ 1.2 KB (AVR), ≈ 2 KB (ARM)
 * SRAM: ≈ 80 B + (task_count × TCB_size)
 */

#include "scheduler.h"
#include "arch/common/hal.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION (from profile)
 *═══════════════════════════════════════════════════════════════════*/

#ifndef NK_MAX_TASKS
#  define NK_MAX_TASKS 8    /**< Maximum number of tasks */
#endif

#ifndef NK_STACK_SIZE
#  define NK_STACK_SIZE 128 /**< Default stack size per task */
#endif

#ifndef NK_QUANTUM_MS
#  define NK_QUANTUM_MS 10  /**< Time slice in milliseconds */
#endif

#ifndef NK_OPT_DAG_WAIT
#  define NK_OPT_DAG_WAIT 0  /**< DAG dependency tracking */
#endif

#ifndef NK_OPT_STACK_GUARD
#  define NK_OPT_STACK_GUARD 0  /**< Stack overflow detection */
#endif

/*═══════════════════════════════════════════════════════════════════
 * SCHEDULER STATE
 *═══════════════════════════════════════════════════════════════════*/

/** Scheduler global state */
static struct {
    nk_tcb_t *tasks[NK_MAX_TASKS];  /**< Task control blocks */
    uint8_t   count;                 /**< Number of active tasks */
    uint8_t   current;               /**< Currently running task index */
    volatile uint8_t quantum;        /**< Remaining time slice */
} nk_sched = {
    .count   = 0,
    .current = 0,
    .quantum = NK_QUANTUM_MS
};

/*═══════════════════════════════════════════════════════════════════
 * STACK MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

#if NK_OPT_STACK_GUARD
/** Stack with guard pages (canaries) */
typedef struct {
    uint32_t guard_lo;               /**< Low guard pattern */
    uint8_t  data[NK_STACK_SIZE];    /**< Actual stack data */
    uint32_t guard_hi;               /**< High guard pattern */
} nk_stack_t;

/** Guard pattern (0xDEADBEEF) */
#define STACK_GUARD_PATTERN 0xDEADBEEF

/** Task stacks with guards */
static nk_stack_t nk_stacks[NK_MAX_TASKS] __attribute__((section(".noinit")));

#else
/** Task stacks without guards (save RAM) */
static uint8_t nk_stacks[NK_MAX_TASKS][NK_STACK_SIZE] __attribute__((section(".noinit")));
#endif

/*═══════════════════════════════════════════════════════════════════
 * SLEEP TIMER MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Update sleep timers for all tasks
 *
 * Called from timer ISR. Decrements sleep_ticks for sleeping tasks
 * and wakes them up when the timer expires.
 */
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

/*═══════════════════════════════════════════════════════════════════
 * TASK SELECTION & SCHEDULING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Find the next task to run
 *
 * Selects the highest-priority ready task using round-robin among
 * tasks of equal priority.
 *
 * @return Index of next task to run
 */
static uint8_t find_next_task(void) {
    uint8_t best  = nk_sched.current;
    uint8_t bestp = 0xFF;

    /* Round-robin search starting from current + 1 */
    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        uint8_t idx = (nk_sched.current + i + 1) % nk_sched.count;
        nk_tcb_t *t = nk_sched.tasks[idx];

        bool ready = (t->state == NK_READY);

#if NK_OPT_DAG_WAIT
        /* Check DAG dependencies */
        ready = ready && (t->deps == 0);
#endif

        /* Select if higher priority (lower number) */
        if (ready && t->priority < bestp) {
            best  = idx;
            bestp = t->priority;
        }
    }

    return best;
}

#if NK_OPT_STACK_GUARD
/**
 * @brief Panic handler for stack overflow
 *
 * Disables interrupts and blinks LED (if GPIO available).
 * Never returns.
 */
static void panic_stack_overflow(void) __attribute__((noreturn));
static void panic_stack_overflow(void) {
    hal_irq_disable();

    /* Blink pattern: SOS in Morse code (... --- ...) */
    /* Platform-specific LED blinking would go here */
    /* For now, just halt */
    for (;;) {
        hal_idle();
    }
}

/**
 * @brief Check stack canaries for current task
 *
 * Verifies guard patterns are intact. Panics if corrupted.
 */
static inline void check_canaries(void) {
    nk_stack_t *stk = &nk_stacks[nk_sched.current];
    if (stk->guard_lo != STACK_GUARD_PATTERN ||
        stk->guard_hi != STACK_GUARD_PATTERN) {
        panic_stack_overflow();
    }
}
#endif /* NK_OPT_STACK_GUARD */

/**
 * @brief Switch to a different task
 *
 * Performs context switch from current task to specified task.
 *
 * @param next Index of task to switch to
 */
static void switch_to(uint8_t next) {
    if (next == nk_sched.current) {
        return;  /* Already running */
    }

#if NK_OPT_STACK_GUARD
    check_canaries();
#endif

    nk_tcb_t *from = nk_sched.tasks[nk_sched.current];
    nk_tcb_t *to   = nk_sched.tasks[next];

    /* Update task states */
    if (from->state == NK_RUNNING) {
        from->state = NK_READY;
    }
    to->state = NK_RUNNING;

    /* Update current task index before context switch */
    nk_sched.current = next;

    /* Perform context switch via HAL */
    hal_context_switch((hal_context_t *)&from->sp, (hal_context_t *)&to->sp);
}

/**
 * @brief Atomically schedule next task
 *
 * Disables interrupts, finds next task, and switches to it.
 * Re-enables interrupts after switch.
 */
static inline void atomic_schedule(void) {
    hal_irq_disable();
    switch_to(find_next_task());
    hal_irq_enable();
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - INITIALIZATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize the scheduler
 *
 * Sets up the system timer for 1 kHz tick and initializes scheduler state.
 * Must be called once at startup before creating any tasks.
 */
void scheduler_init(void) {
#if NK_OPT_STACK_GUARD
    /* Initialize stack guards */
    for (uint8_t i = 0; i < NK_MAX_TASKS; ++i) {
        nk_stacks[i].guard_lo = STACK_GUARD_PATTERN;
        nk_stacks[i].guard_hi = STACK_GUARD_PATTERN;
    }
#endif

    /* Initialize system timer for 1 kHz tick */
    hal_timer_init(1000);  /* 1000 Hz = 1 kHz */

    /* Reset scheduler state */
    nk_sched.count = 0;
    nk_sched.current = 0;
    nk_sched.quantum = NK_QUANTUM_MS;
}

/* Aliases for compatibility */
void nk_sched_init(void) __attribute__((alias("scheduler_init")));
void nk_init(void) __attribute__((alias("scheduler_init")));

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - TASK CREATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Create a new task
 *
 * @param tcb Pointer to task control block (caller-allocated)
 * @param entry Task entry point function
 * @param prio Priority (0 = highest, 63 = lowest)
 * @param stack Pointer to stack buffer (or NULL for internal stack)
 * @param stack_len Stack size in bytes
 * @return true on success, false on failure
 */
bool nk_task_create(nk_tcb_t *tcb,
                    nk_task_fn entry,
                    uint8_t prio,
                    void *stack,
                    size_t stack_len) {
    if (!tcb || !entry) {
        return false;
    }

    if (nk_sched.count >= NK_MAX_TASKS) {
        return false;  /* Too many tasks */
    }

    if (stack_len < 64) {
        return false;  /* Stack too small */
    }

    /* Use internal stack if not provided */
    if (!stack) {
#if NK_OPT_STACK_GUARD
        stack = nk_stacks[nk_sched.count].data;
        stack_len = NK_STACK_SIZE;
#else
        stack = nk_stacks[nk_sched.count];
        stack_len = NK_STACK_SIZE;
#endif
    }

    /* Initialize context via HAL */
    hal_context_init((hal_context_t *)&tcb->sp, entry, stack, stack_len);

    /* Initialize TCB */
    tcb->state = NK_READY;
    tcb->priority = (prio & 0x3F);  /* 6-bit priority */
    tcb->pid = nk_sched.count;
    tcb->sleep_ticks = 0;

#if NK_OPT_DAG_WAIT
    tcb->deps = 0;
#endif

    /* Add to task list */
    hal_irq_disable();
    nk_sched.tasks[nk_sched.count++] = tcb;
    hal_irq_enable();

    return true;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - SCHEDULER CONTROL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Start the scheduler (never returns)
 *
 * Enables interrupts and switches to the first ready task.
 */
void scheduler_run(void) __attribute__((noreturn));
void scheduler_run(void) {
    hal_irq_enable();
    switch_to(find_next_task());

    /* Should never reach here */
    for (;;) {
        hal_idle();
    }
}

/* Aliases for compatibility */
void nk_sched_run(void) __attribute__((alias("scheduler_run")));
void nk_start(void) __attribute__((alias("scheduler_run")));

/**
 * @brief Yield the processor
 *
 * Voluntarily gives up the CPU to allow other tasks to run.
 */
void nk_yield(void) {
    hal_irq_disable();
    nk_sched.quantum = 0;  /* Force reschedule */
    atomic_schedule();
}

/**
 * @brief Sleep for specified milliseconds
 *
 * Suspends the calling task for at least the specified time.
 *
 * @param ms Milliseconds to sleep
 */
void nk_sleep(uint16_t ms) {
    hal_irq_disable();

    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->state = NK_SLEEPING;
    t->sleep_ticks = ms;

    atomic_schedule();
}

/**
 * @brief Get current task ID
 *
 * @return Task ID (0 to max_tasks-1)
 */
uint8_t nk_current_tid(void) {
    return nk_sched.current;
}

/**
 * @brief Switch to specific task (internal use)
 *
 * @param tid Task ID to switch to
 */
void nk_switch_to(uint8_t tid) {
    if (tid >= nk_sched.count) {
        return;
    }

    hal_irq_disable();
    switch_to(tid);
    hal_irq_enable();
}

/**
 * @brief Terminate current task
 *
 * Marks the task as terminated and yields to scheduler.
 *
 * @param status Exit status (ignored for now)
 */
void nk_task_exit(int status) __attribute__((noreturn));
void nk_task_exit(int status) {
    (void)status;

    hal_irq_disable();

    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->state = NK_TERMINATED;

    /* Find and switch to next task */
    atomic_schedule();

    /* Should never reach here */
    for (;;) {
        hal_idle();
    }
}

/*═══════════════════════════════════════════════════════════════════
 * TIMER TICK HANDLER
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief System tick handler
 *
 * Called by HAL timer ISR at 1 kHz. Updates sleep timers and
 * performs time-slice preemption.
 *
 * This function must be called from the timer ISR via HAL.
 */
void hal_timer_tick_handler(void) {
    /* Update sleep timers */
    update_sleep_timers();

    /* Decrement quantum */
    if (--nk_sched.quantum == 0) {
        nk_sched.quantum = NK_QUANTUM_MS;

        /* Time slice expired - reschedule */
        uint8_t next = find_next_task();
        if (next != nk_sched.current) {
            switch_to(next);
        }
    }
}

/*═══════════════════════════════════════════════════════════════════
 * OPTIONAL: DAG DEPENDENCY TRACKING
 *═══════════════════════════════════════════════════════════════════*/

#if NK_OPT_DAG_WAIT

/**
 * @brief Wait for DAG dependencies
 *
 * Blocks the calling task until all dependencies are signaled.
 *
 * @param deps Number of dependencies to wait for
 */
void nk_task_wait(uint8_t deps) {
    hal_irq_disable();

    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->deps = deps;
    t->state = NK_BLOCKED;

    atomic_schedule();
}

/**
 * @brief Signal a waiting task
 *
 * Decrements the dependency count for a task. If count reaches zero,
 * the task becomes ready.
 *
 * @param tid Task ID to signal
 */
void nk_task_signal(uint8_t tid) {
    if (tid >= nk_sched.count) {
        return;
    }

    hal_irq_disable();

    nk_tcb_t *t = nk_sched.tasks[tid];
    if (t->deps && --t->deps == 0 && t->state == NK_BLOCKED) {
        t->state = NK_READY;
    }

    hal_irq_enable();
}

#endif /* NK_OPT_DAG_WAIT */

/*═══════════════════════════════════════════════════════════════════
 * END OF FILE
 *═══════════════════════════════════════════════════════════════════*/
