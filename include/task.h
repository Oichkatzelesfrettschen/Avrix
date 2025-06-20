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
 * @brief Run the scheduler loop.
 */
void scheduler_run(void);

/**
 * @brief Return the ID of the currently running task.
 */
uint8_t scheduler_current_tid(void);

/**
 * @brief Immediately switch to the specified task.
 * @param tid Target task ID.
 */
void scheduler_switch_to(uint8_t tid);

#ifdef __cplusplus
}
#endif

#endif // AVR_TASK_H
