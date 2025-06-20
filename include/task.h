#ifndef AVR_TASK_H
#define AVR_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file task.h
 * @brief Minimal task control block and scheduler interface.
 */

/** Stack pointer type for tasks. */
typedef uint16_t sp_t;

/**
 * Task state enumeration.
 */
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
} task_state_t;

/** Task Control Block (TCB). */
typedef struct {
    sp_t sp;             /**< Saved stack pointer. */
    task_state_t state;  /**< Current task state. */
    uint8_t priority;    /**< Priority (0-63). */
    uint8_t deps;        /**< Outstanding dependencies for DAG scheduling. */
} tcb_t;

/** Maximum number of tasks supported. */
#define MAX_TASKS 10

/**
 * @brief Initialise the scheduler.
 */
void scheduler_init(void);

/**
 * @brief Add a task to the scheduler.
 * @param tcb  Pointer to task control block.
 * @param entry Function pointer to task entry.
 * @param stack Top of stack memory.
 */
void scheduler_add_task(tcb_t *tcb, void (*entry)(void), void *stack);

/**
 * @brief Yield control to the next ready task.
 */
void scheduler_yield(void);

/**
 * @brief Run the scheduler loop.
 */
void scheduler_run(void);

/**
 * @brief Mark a blocked task as ready when a dependency completes.
 */
void scheduler_signal(tcb_t *tcb);

/**
 * @brief Block a task until \c deps dependencies are satisfied.
 */
void scheduler_block(tcb_t *tcb, uint8_t deps);

#ifdef __cplusplus
}
#endif

#endif // AVR_TASK_H
