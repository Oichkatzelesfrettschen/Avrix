/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*────────────────────────── task.h ───────────────────────────────
   µ-UNIX – Task & scheduler interface (Legacy Shim)
   ----------------------------------------------------------------*/
#ifndef NK_TASK_H
#define NK_TASK_H

#include "kernel/sched/scheduler.h"
#include "avrix-config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy compatibility mappings */
#ifndef NK_MAX_TASKS
#define NK_MAX_TASKS CONFIG_KERNEL_TASK_MAX
#endif

#ifndef NK_STACK_SIZE
#define NK_STACK_SIZE CONFIG_KERNEL_STACK_SIZE
#endif

/* Map nk_cur_tid -> nk_current_tid */
static inline uint8_t nk_cur_tid(void) {
    return nk_current_tid();
}

/* Legacy nk_task_add wrapper around nk_task_create */
static inline void nk_task_add(nk_tcb_t *tcb,
                               void (*entry)(void),
                               void *stack_top,
                               uint8_t prio,
                               uint8_t class_id) {
    (void)stack_top; /* Unused */
    (void)class_id;  /* Unused in new scheduler for now */
    nk_task_create(tcb, entry, prio, NULL, 0);
}

#ifdef __cplusplus
}
#endif

#endif /* NK_TASK_H */
