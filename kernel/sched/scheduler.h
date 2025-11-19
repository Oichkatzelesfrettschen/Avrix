/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file scheduler.h
 * @brief Portable Scheduler Interface
 *
 * Provides a POSIX-compliant task scheduler for embedded systems.
 * This header defines the scheduler API and task control block structure.
 */

#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*═══════════════════════════════════════════════════════════════════
 * TASK STATES
 *═══════════════════════════════════════════════════════════════════*/

/** Task state enumeration */
typedef enum {
    NK_READY = 0,       /**< Task is ready to run */
    NK_RUNNING,         /**< Task is currently running */
    NK_SLEEPING,        /**< Task is sleeping */
    NK_BLOCKED,         /**< Task is blocked (waiting) */
    NK_TERMINATED       /**< Task has terminated */
} nk_state_t;

/*═══════════════════════════════════════════════════════════════════
 * TASK CONTROL BLOCK
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Task Control Block (TCB)
 *
 * Stores all state for a single task. Size is kept minimal (8-10 bytes)
 * to fit many tasks in constrained SRAM.
 */
typedef struct nk_tcb {
    uint16_t sp;                /**< Saved stack pointer */
    uint8_t  state;             /**< Task state (nk_state_t) */
    uint8_t  priority;          /**< Priority (0 = highest, 63 = lowest) */
    uint8_t  pid;               /**< Task ID (0 to max-1) */
    uint16_t sleep_ticks;       /**< Remaining sleep time (ms) */

#if NK_OPT_DAG_WAIT
    uint8_t  deps;              /**< DAG dependency count */
#endif
} nk_tcb_t;

/**
 * @brief Task entry point function type
 */
typedef void (*nk_task_fn)(void);

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - SCHEDULER CONTROL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize the scheduler
 *
 * Must be called once at startup before creating tasks or starting
 * the scheduler.
 */
void scheduler_init(void);

/**
 * @brief Start the scheduler (never returns)
 *
 * Enables interrupts and begins executing tasks. Does not return.
 */
void scheduler_run(void) __attribute__((noreturn));

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - TASK MANAGEMENT
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
                    size_t stack_len);

/**
 * @brief Yield the processor
 *
 * Voluntarily gives up the CPU to allow other tasks to run.
 */
void nk_yield(void);

/**
 * @brief Sleep for specified milliseconds
 *
 * Suspends the calling task for at least the specified time.
 *
 * @param ms Milliseconds to sleep
 */
void nk_sleep(uint16_t ms);

/**
 * @brief Get current task ID
 *
 * @return Task ID (0 to max_tasks-1)
 */
uint8_t nk_current_tid(void);

/**
 * @brief Terminate current task
 *
 * Marks the task as terminated and yields to scheduler.
 *
 * @param status Exit status (ignored for now)
 */
void nk_task_exit(int status) __attribute__((noreturn));

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
void nk_task_wait(uint8_t deps);

/**
 * @brief Signal a waiting task
 *
 * Decrements the dependency count for a task.
 *
 * @param tid Task ID to signal
 */
void nk_task_signal(uint8_t tid);

#endif /* NK_OPT_DAG_WAIT */

/*═══════════════════════════════════════════════════════════════════
 * COMPATIBILITY ALIASES (for legacy code)
 *═══════════════════════════════════════════════════════════════════*/

/** @brief Alias for scheduler_init() */
void nk_sched_init(void);

/** @brief Alias for scheduler_run() */
void nk_sched_run(void) __attribute__((noreturn));

/** @brief Alias for scheduler_init() */
void nk_init(void);

/** @brief Alias for scheduler_run() */
void nk_start(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SCHEDULER_H */
