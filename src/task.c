/*═══════════════════════════════════════════════════════════════════
 * nk_task.c - Portable C23 scheduler tuned for ATmega128
 *
 * Implements a tiny preemptive, priority based round-robin scheduler
 * with optional dependency tracking and stack overflow detection. The
 * design uses modern language constructs yet keeps code and memory
 * usage minimal: a complete kernel fits comfortably within 32 kB of
 * flash and requires less than 1 kB of SRAM.
 *═══════════════════════════════════════════════════════════════════*/

#include "nk_task.h"
#include "door.h"                 /* door_t matrix lives in .noinit   */

#if NK_OPT_STACK_GUARD
#  include "memguard.h"
#endif

#include <string.h>
#include <stdckdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

/*──────────────────────── Scheduler globals ─────────────────────*/

static struct {
    nk_tcb_t *tasks[NK_MAX_TASKS];
    uint8_t   count;
    uint8_t   current;
    volatile uint8_t quantum_left;
} nk_sched;

/* IPC slabs – same layout as before for compatibility.          */
door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS] __attribute__((section(".noinit")));

/* Optional per-task stacks with guards placed in .noinit.       */
#if NK_OPT_STACK_GUARD
typedef struct {
    uint32_t guard_bottom;
    uint8_t  stack[NK_STACK_SIZE];
    uint32_t guard_top;
} nk_stack_t;

static nk_stack_t nk_stacks[NK_MAX_TASKS]
    __attribute__((section(".noinit")));
#else
static uint8_t nk_stacks[NK_MAX_TASKS][NK_STACK_SIZE]
    __attribute__((section(".noinit")));
#endif

/* Assembly context switch implemented separately.               */
extern void _nk_switch_context(nk_sp_t *from_sp, nk_sp_t to_sp);

/*───────────────────────── Local helpers ───────────────────────*/

static void update_sleep_timers(void)
{
    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        nk_tcb_t *t = nk_sched.tasks[i];
        if (t->state == NK_SLEEPING && t->sleep_ticks) {
            --t->sleep_ticks;
            if (t->sleep_ticks == 0)
                t->state = NK_READY;
        }
    }
}

static uint8_t find_next_task(void)
{
    uint8_t best_idx = nk_sched.current;
    uint8_t best_pri = 0xFF;

    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        uint8_t idx;
        if (ckd_add(&idx, nk_sched.current, (uint8_t)(i + 1)))
            idx = 0;
        idx %= nk_sched.count;
        nk_tcb_t *t = nk_sched.tasks[idx];

        bool ready = (t->state == NK_READY);
#if NK_OPT_DAG_WAIT
        ready = ready && (t->deps == 0);
#endif
        if (ready && t->priority < best_pri) {
            best_idx = idx;
            best_pri = t->priority;
        }
    }
    return best_idx;
}

static void switch_to_task(uint8_t next)
{
    if (next == nk_sched.current)
        return;

#if NK_OPT_STACK_GUARD
    nk_stack_t *stk = &nk_stacks[nk_sched.current];
    if (stk->guard_bottom != STACK_GUARD_PATTERN ||
        stk->guard_top    != STACK_GUARD_PATTERN) {
        cli();
        for (;;) {
            PORTB ^= _BV(PB5);
            for (volatile uint32_t d=0; d<100000; ++d);
        }
    }
#endif

    nk_tcb_t *restrict from = nk_sched.tasks[nk_sched.current];
    nk_tcb_t *restrict to   = nk_sched.tasks[next];

    switch (from->state) {
    case NK_RUNNING:
        from->state = NK_READY;
        break;
    default:
        /* NK_SLEEPING or NK_BLOCKED already set */
        break;
    }
    to->state = NK_RUNNING;
    nk_sched.current = next;

    _nk_switch_context(&from->sp, to->sp);
}

/* Call scheduler with interrupts masked to avoid race conditions */
static void schedule_next(void)
{
    uint8_t n = find_next_task();
    switch_to_task(n);
}

static inline void atomic_schedule(void)
{
    cli();
    schedule_next();
    sei();
}

/*─────────────────────── Public interface ──────────────────────*/

void nk_init(void)
{
    nk_sched.count = 0;
    nk_sched.current = 0;
    nk_sched.quantum_left = NK_QUANTUM_MS;

#if NK_OPT_STACK_GUARD
    for (size_t i = 0; i < NK_MAX_TASKS; ++i) {
        nk_stacks[i].guard_bottom = STACK_GUARD_PATTERN;
        nk_stacks[i].guard_top    = STACK_GUARD_PATTERN;
    }
#endif

    /* TIMER0 CTC → 1 kHz  (16 MHz / 64 / 250) */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);
    OCR0A  = (uint8_t)(F_CPU / 64 / 1000 - 1);
    TIMSK0 = _BV(OCIE0A);
}

bool nk_task_create(nk_tcb_t *tcb, nk_task_fn entry,
                    uint8_t priority, void *stack_base,
                    size_t stack_size)
{
    if (!tcb || !entry || !stack_base)
        return false;
    if (nk_sched.count >= NK_MAX_TASKS)
        return false;
    if (stack_size < 32 || stack_size > NK_STACK_SIZE)
        return false;

    uint8_t *sp = (uint8_t*)stack_base + stack_size;
    *--sp = (uint16_t)entry & 0xFF;
    *--sp = (uint16_t)entry >> 8;
    *--sp = 0x80;                       /* SREG with I flag */
    for (uint8_t i = 0; i < 32; ++i)
        *--sp = 0;

    *tcb = (nk_tcb_t){
        .sp          = sp,
        .state       = NK_READY,
        .priority    = (uint8_t)(priority & 0x3F),
        .pid         = nk_sched.count,
        .deps        = 0,
        .sleep_ticks = 0
    };

    cli();
    nk_sched.tasks[nk_sched.count++] = tcb;
    sei();
    return true;
}

void nk_start(void)
{
    sei();
    schedule_next();
    __builtin_unreachable();
}

void nk_yield(void)
{
    cli();
    nk_sched.quantum_left = 0;
    atomic_schedule();
}

void nk_sleep(uint16_t ms)
{
    cli();
    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->state = NK_SLEEPING;
    t->sleep_ticks = ms;
    atomic_schedule();
}

uint8_t nk_current_tid(void)
{
    return nk_sched.current;
}

void nk_switch_to(uint8_t tid)
{
    cli();
    if (tid < nk_sched.count)
        switch_to_task(tid);
    sei();
}

#if NK_OPT_DAG_WAIT
void nk_task_wait(uint8_t deps)
{
    cli();
    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->deps = deps;
    t->state = NK_BLOCKED;
    atomic_schedule();
}

void nk_task_signal(uint8_t tid)
{
    if (tid >= nk_sched.count)
        return;

    cli();
    nk_tcb_t *t = nk_sched.tasks[tid];
    if (t->deps) {
        --t->deps;
        if (t->deps == 0 && t->state == NK_BLOCKED)
            t->state = NK_READY;
    }
    sei();
}
#endif

/*──────────────────── Timer ISR for preemption ──────────────────*/
/*
 * TIMER0_COMPA interrupt service routine
 * --------------------------------------
 * The ISR is declared ISR_NAKED to retain full control over the
 * prologue/epilogue. Only r24 and SREG are touched here; the actual
 * housekeeping is delegated to nk_timer0_handler() in C for clarity.
 */
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    asm volatile(
        "push r24            \n\t"
        "in   r24, __SREG__  \n\t"
        "push r24            \n\t"
        ::: "memory");
    nk_timer0_handler();
    asm volatile(
        "pop  r24            \n\t"
        "out  __SREG__, r24  \n\t"
        "pop  r24            \n\t"
        "reti                \n"
        ::: "memory");
}

static void nk_timer0_handler(void)
{
    update_sleep_timers();
    if (--nk_sched.quantum_left == 0) {
        nk_sched.quantum_left = NK_QUANTUM_MS;
        schedule_next();
    }
}

