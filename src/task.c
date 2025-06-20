#include "task.h"
#include "door.h"
#include <avr/interrupt.h>
#include <avr/io.h>

static tcb_t *task_list[MAX_TASKS];
static volatile uint8_t task_count;
static volatile uint8_t current_task;

/* Door descriptors for the current task. Placed in .noinit. */
door_t door_vec[DOOR_SLOTS] __attribute__((section(".noinit")));

/* Save/restore all CPU registers for a preemptive context switch. */
#define SAVE_CONTEXT()                             \
    __asm__ __volatile__(                         \
        "push r0\n\t"                               \
        "in   r0, __SREG__\n\t"                    \
        "cli\n\t"                                   \
        "push r0\n\t"                               \
        "push r1\n\t"                               \
        "clr  r1\n\t"                               \
        "push r2\n\t"                               \
        "push r3\n\t"                               \
        "push r4\n\t"                               \
        "push r5\n\t"                               \
        "push r6\n\t"                               \
        "push r7\n\t"                               \
        "push r8\n\t"                               \
        "push r9\n\t"                               \
        "push r10\n\t"                              \
        "push r11\n\t"                              \
        "push r12\n\t"                              \
        "push r13\n\t"                              \
        "push r14\n\t"                              \
        "push r15\n\t"                              \
        "push r16\n\t"                              \
        "push r17\n\t"                              \
        "push r18\n\t"                              \
        "push r19\n\t"                              \
        "push r20\n\t"                              \
        "push r21\n\t"                              \
        "push r22\n\t"                              \
        "push r23\n\t"                              \
        "push r24\n\t"                              \
        "push r25\n\t"                              \
        "push r26\n\t"                              \
        "push r27\n\t"                              \
        "push r28\n\t"                              \
        "push r29\n\t"                              \
        "push r30\n\t"                              \
        "push r31\n\t"                              \
    )

#define RESTORE_CONTEXT()                          \
    __asm__ __volatile__(                         \
        "pop r31\n\t"                               \
        "pop r30\n\t"                               \
        "pop r29\n\t"                               \
        "pop r28\n\t"                               \
        "pop r27\n\t"                               \
        "pop r26\n\t"                               \
        "pop r25\n\t"                               \
        "pop r24\n\t"                               \
        "pop r23\n\t"                               \
        "pop r22\n\t"                               \
        "pop r21\n\t"                               \
        "pop r20\n\t"                               \
        "pop r19\n\t"                               \
        "pop r18\n\t"                               \
        "pop r17\n\t"                               \
        "pop r16\n\t"                               \
        "pop r15\n\t"                               \
        "pop r14\n\t"                               \
        "pop r13\n\t"                               \
        "pop r12\n\t"                               \
        "pop r11\n\t"                               \
        "pop r10\n\t"                               \
        "pop r9\n\t"                                \
        "pop r8\n\t"                                \
        "pop r7\n\t"                                \
        "pop r6\n\t"                                \
        "pop r5\n\t"                                \
        "pop r4\n\t"                                \
        "pop r3\n\t"                                \
        "pop r2\n\t"                                \
        "pop r1\n\t"                                \
        "pop r0\n\t"                                \
        "out  __SREG__, r0\n\t"                     \
        "pop r0\n\t"                                \
    )

/**
 * Simple round-robin scheduler with fixed time slice.
 */
void scheduler_init(void) {
    task_count = 0;
    current_task = 0;

    /* Configure Timer0 for a 1 ms tick using CTC mode. */
    TCCR0A = (1 << WGM01);                 /* CTC mode */
    TCCR0B = (1 << CS01) | (1 << CS00);    /* prescaler 64 */
    OCR0A = (F_CPU / 64 / 1000) - 1;       /* compare value */
    TIMSK0 = (1 << OCIE0A);                /* enable interrupt */
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
    tcb->deps = 0;

    task_list[task_count++] = tcb;
}

void scheduler_block(tcb_t *tcb, uint8_t deps)
{
    tcb->deps = deps;
    tcb->state = TASK_BLOCKED;
}

void scheduler_signal(tcb_t *tcb)
{
    if (tcb->deps && --tcb->deps == 0)
        tcb->state = TASK_READY;
}

static uint8_t select_next_ready(uint8_t from)
{
    uint8_t idx = from;
    uint8_t best = from;
    uint8_t prio = 0xFF;

    for (uint8_t i = 0; i < task_count; ++i) {
        idx = (idx + 1) % task_count;
        tcb_t *t = task_list[idx];
        if (t->state == TASK_READY && t->deps == 0 && t->priority <= prio) {
            prio = t->priority;
            best = idx;
        }
    }
    return best;
}

/* Common task selection and stack swap. */
static inline void schedule_core(uint16_t sp)
{
    tcb_t *prev = task_list[current_task];
    prev->sp = sp;
    prev->state = TASK_READY;

    current_task = select_next_ready(current_task);
    tcb_t *next = task_list[current_task];
    next->state = TASK_RUNNING;

    sp = next->sp;
    __asm__ __volatile__(
        "out  __SP_L__, %A0\n"
        "out  __SP_H__, %B0\n"
        :: "r" (sp));
}

/**
 * @brief Yield control to the highest-priority ready task.
 */
void scheduler_yield(void) __attribute__((naked));
void scheduler_yield(void)
{
    SAVE_CONTEXT();

    /* Capture current stack pointer after saving registers. */
    uint16_t sp;
    __asm__ __volatile__(
        "in   %A0, __SP_L__\n"
        "in   %B0, __SP_H__\n"
        : "=r" (sp));

    schedule_core(sp);

    RESTORE_CONTEXT();
    __asm__ __volatile__("ret");
}

void scheduler_run(void) {
    if (task_count == 0) {
        return;
    }

    task_list[current_task]->state = TASK_RUNNING;
    sei();
    scheduler_yield();
    for (;;);
}

/* Timer0 compare interrupt drives periodic task switching. */
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    SAVE_CONTEXT();

    /* Preserve the interrupted task state and stack pointer. */
    uint16_t sp;
    __asm__ __volatile__(
        "in   %A0, __SP_L__\n"
        "in   %B0, __SP_H__\n"
        : "=r" (sp));

    schedule_core(sp);

    RESTORE_CONTEXT();
    __asm__ __volatile__("reti");
}

