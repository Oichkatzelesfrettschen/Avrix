#include "task.h"
#include "door.h"
#include "memguard.h"
#include <string.h>
#include <avr/interrupt.h>

static tcb_t *task_list[MAX_TASKS];
static uint8_t task_count;
static uint8_t current_task;

/* Stacks guarded by sentinels defined in memguard.h. */
static uint8_t stacks[MAX_TASKS][TASK_STACK_SIZE + 2 * GUARD_BYTES];

static void check_stacks(void)
{
    for (uint8_t i = 0; i < task_count; ++i) {
        if (!check_guard(stacks[i], sizeof(stacks[i]))) {
            /* fatal overflow detected */
            while (1)
                ;
        }
    }
}

/* Door descriptors for the current task. Placed in .noinit. */
door_t door_vec[DOOR_SLOTS] __attribute__((section(".noinit")));

/**
 * Simple round-robin scheduler with fixed time slice.
 */
void scheduler_init(void) {
    task_count = 0;
    current_task = 0;
}

void scheduler_add_task(tcb_t *tcb, void (*entry)(void), void *stack)
{
    if (task_count >= MAX_TASKS) {
        return;
    }

    (void)stack; /* stacks are allocated internally */

    uint8_t *region = stacks[task_count];
    const size_t total = sizeof(stacks[task_count]);
    guard_init(region, total);
    memset(region + GUARD_BYTES, 0, TASK_STACK_SIZE);

    /* stack grows downward; start just below the top guard */
    uint8_t *sp = region + GUARD_BYTES + TASK_STACK_SIZE;
    *--sp = (uint16_t)entry & 0xFF;  /* return address */
    *--sp = (uint16_t)entry >> 8;

    tcb->sp = (sp_t)sp;
    tcb->state = TASK_READY;
    tcb->priority = 0;

    task_list[task_count++] = tcb;
}

static inline void context_switch(tcb_t *from, tcb_t *to) {
    uint16_t current;
    __asm__ __volatile__(
        "in   %A0, __SP_L__\n"
        "in   %B0, __SP_H__\n"
        : "=r" (current));
    from->sp = current;
    __asm__ __volatile__(
        "out  __SP_L__, %A0\n"
        "out  __SP_H__, %B0\n"
        :
        : "r" (to->sp));
}

void scheduler_run(void) {
    if (task_count == 0) {
        return;
    }

    tcb_t *prev = task_list[0];
    while (1) {
        check_stacks();
        tcb_t *next = task_list[current_task];
        if (next->state == TASK_READY) {
            next->state = TASK_RUNNING;
            context_switch(prev, next);
            prev->state = TASK_READY;
            prev = next;
        }
        current_task = (current_task + 1) % task_count;
    }
}

