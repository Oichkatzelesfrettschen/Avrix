#include "task.h"
#include <avr/interrupt.h>

static tcb_t *task_list[MAX_TASKS];
static uint8_t task_count;
static uint8_t current_task;

/**
 * Simple round-robin scheduler with fixed time slice.
 */
void scheduler_init(void) {
    task_count = 0;
    current_task = 0;
}

void scheduler_add_task(tcb_t *tcb, void (*entry)(void), void *stack) {
    if (task_count >= MAX_TASKS) {
        return;
    }

    // Initialise stack frame for new task.
    uint8_t *sp = (uint8_t *)stack;
    *--sp = (uint16_t)entry & 0xFF;  // return address
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

