/*═══════════════════════════════════════════════════════════════════
 * nk_task.c – tiny pre-emptive scheduler for ATmega328P
 *
 *  • priority-based round-robin, 1 kHz tick
 *  • optional DAG dependency counters  (NK_OPT_DAG_WAIT)
 *  • optional stack-overflow canaries  (NK_OPT_STACK_GUARD)
 *
 *  Flash  ≈ 1.2 kB  (avr-gcc-14 -Oz -flto)
 *  SRAM   ≈   80 B  (globals + quantum counter)
 *═══════════════════════════════════════════════════════════════════*/

#include "nk_task.h"
#include "door.h"          /* door_vec lives in .noinit */

#if NK_OPT_STACK_GUARD
#  include "memguard.h"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

/*───────────────────── 1. Scheduler globals ─────────────────────*/

static struct {
    nk_tcb_t *tasks[NK_MAX_TASKS];
    uint8_t   count;
    uint8_t   current;
    volatile uint8_t quantum;
} nk_sched = {
    .count   = 0,
    .current = 0,
    .quantum = NK_QUANTUM_MS
};

/* Door RPC descriptors (defined in door.c) ----------------------*/
extern door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

/* Optional guarded stacks ---------------------------------------*/
#if NK_OPT_STACK_GUARD
typedef struct {
    uint32_t guard_lo;
    uint8_t  data[NK_STACK_SIZE];
    uint32_t guard_hi;
} nk_stack_t;

static nk_stack_t nk_stacks[NK_MAX_TASKS]
    __attribute__((section(".noinit")));
#else
static uint8_t nk_stacks[NK_MAX_TASKS][NK_STACK_SIZE]
    __attribute__((section(".noinit")));
#endif

/* Hand-rolled context switch (ASM) ------------------------------*/
extern void _nk_switch_context(nk_sp_t *save_sp, nk_sp_t load_sp);
extern void _nk_switch_task(nk_sp_t *save_sp,
                            nk_sp_t  load_sp,
                            uint8_t *current_ptr,
                            uint8_t  next_tid);

/*───────────────────── 2. Helper routines ───────────────────────*/

static void update_sleep_timers(void)
{
    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        nk_tcb_t *t = nk_sched.tasks[i];
        if (t->state == NK_SLEEPING && t->sleep_ticks) {
            if (--t->sleep_ticks == 0)
                t->state = NK_READY;
        }
    }
}

static uint8_t find_next_task(void)
{
    uint8_t best  = nk_sched.current;
    uint8_t bestp = 0xFF;

    for (uint8_t i = 0; i < nk_sched.count; ++i) {
        uint8_t idx = nk_sched.current + (uint8_t)(i + 1);
        idx %= nk_sched.count;

        nk_tcb_t *t = nk_sched.tasks[idx];
        bool ready  = (t->state == NK_READY);
#if NK_OPT_DAG_WAIT
        ready = ready && (t->deps == 0);
#endif
        if (ready && t->priority < bestp) {
            best  = idx;
            bestp = t->priority;
        }
    }
    return best;
}

#if NK_OPT_STACK_GUARD
static void panic_stack_overflow(void) __attribute__((noreturn));
static void panic_stack_overflow(void)
{
    cli();
    DDRB  |= _BV(PB5);
    for (;;){
        PORTB ^= _BV(PB5);
        for (volatile uint32_t d = 0; d < 40000; ++d);
    }
}

static inline void check_canaries(void)
{
    nk_stack_t *stk = &nk_stacks[nk_sched.current];
    if (stk->guard_lo != STACK_GUARD_PATTERN ||
        stk->guard_hi != STACK_GUARD_PATTERN)
        panic_stack_overflow();
}
#endif /* NK_OPT_STACK_GUARD */

static void switch_to(uint8_t next)
{
    if (next == nk_sched.current)
        return;

#if NK_OPT_STACK_GUARD
    check_canaries();
#endif

    nk_tcb_t *from = nk_sched.tasks[nk_sched.current];
    nk_tcb_t *to   = nk_sched.tasks[next];

    if (from->state == NK_RUNNING)
        from->state = NK_READY;
    to->state       = NK_RUNNING;

    _nk_switch_task(&from->sp, to->sp, &nk_sched.current, next);
}

static inline void atomic_schedule(void)
{
    cli();
    switch_to(find_next_task());
    sei();
}

/*───────────────────── 3. Public API  ───────────────────────────*/

void nk_init(void)
{
#if NK_OPT_STACK_GUARD
    for (nk_stack_t *s = nk_stacks; s < nk_stacks + NK_MAX_TASKS; ++s) {
        s->guard_lo = STACK_GUARD_PATTERN;
        s->guard_hi = STACK_GUARD_PATTERN;
    }
#endif
    /* 1 kHz tick: 16 MHz / 64 / 250 */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);
    OCR0A  = (uint8_t)(F_CPU / 64 / 1000 - 1);
    TIMSK0 = _BV(OCIE0A);
}

bool nk_task_create(nk_tcb_t *tcb,
                    nk_task_fn entry,
                    uint8_t    prio,
                    void      *stack_base,
                    size_t     stack_len)
{
    if (!tcb || !entry || !stack_base)
        return false;
    if (nk_sched.count >= NK_MAX_TASKS)
        return false;
    if (stack_len < 32 || stack_len > NK_STACK_SIZE)
        return false;

    /* initial frame: PC, SREG, 32 regs */
    uint8_t *sp = (uint8_t*)stack_base + stack_len;
    *--sp = (uint16_t)entry & 0xFF;
    *--sp = (uint16_t)entry >> 8;
    *--sp = 0x80;                       /* SREG with I-flag */
    memset(sp - 32, 0, 32);
    sp -= 32;

    *tcb = (nk_tcb_t){
        .sp          = sp,
        .state       = NK_READY,
        .priority    = (uint8_t)(prio & 0x3F),
        .pid         = nk_sched.count,
        .deps        = 0,
        .sleep_ticks = 0
    };

    cli();
    nk_sched.tasks[nk_sched.count++] = tcb;
    sei();
    return true;
}

/* Legacy alias kept for compatibility --------------------------------*/
bool nk_task_add(nk_tcb_t *t, void (*e)(void),
                 void *stack_top, uint8_t p, uint8_t cls)
    __attribute__((alias("nk_task_create")));

/* Start the scheduler (never returns) --------------------------------*/
void nk_start(void)
{
    sei();
    switch_to(find_next_task());
    __builtin_unreachable();
}

void nk_yield(void)
{
    cli();
    nk_sched.quantum = 0;
    atomic_schedule();
}

void nk_sleep(uint16_t ms)
{
    cli();
    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->state       = NK_SLEEPING;
    t->sleep_ticks = ms;
    atomic_schedule();
}

uint8_t nk_current_tid(void) { return nk_sched.current; }

void nk_switch_to(uint8_t tid)
{
    if (tid >= nk_sched.count)
        return;
    cli();
    switch_to(tid);
    sei();
}

#if NK_OPT_DAG_WAIT
void nk_task_wait(uint8_t deps)
{
    cli();
    nk_tcb_t *t = nk_sched.tasks[nk_sched.current];
    t->deps  = deps;
    t->state = NK_BLOCKED;
    atomic_schedule();
}

void nk_task_signal(uint8_t tid)
{
    if (tid >= nk_sched.count)
        return;
    cli();
    nk_tcb_t *t = nk_sched.tasks[tid];
    if (t->deps && --t->deps == 0 && t->state == NK_BLOCKED)
        t->state = NK_READY;
    sei();
}
#endif /* DAG */

/*───────────────────── 4. 1 kHz timer ISR ───────────────────────*/
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    asm volatile(
        "push  r24            \n\t"
        "in    r24, __SREG__  \n\t"
        "push  r24            \n\t"
        ::: "memory");

    update_sleep_timers();
    if (--nk_sched.quantum == 0) {
        nk_sched.quantum = NK_QUANTUM_MS;
        switch_to(find_next_task());
    }

    asm volatile(
        "pop   r24            \n\t"
        "out   __SREG__, r24  \n\t"
        "pop   r24            \n\t"
        "reti                 \n\t"
        ::: "memory");
}

/*────────────── Door service ISR – server-side dispatch ─────────────*/
#ifndef DOORSERV_vect
#  define DOORSERV_vect _VECTOR(32)
#endif

ISR(DOORSERV_vect, ISR_NAKED)
{
    asm volatile(
        "push  r24            \n\t"
        "in    r24, __SREG__  \n\t"
        "push  r24            \n\t"
        ::: "memory");

    switch_to(find_next_task());

    asm volatile(
        "pop   r24            \n\t"
        "out   __SREG__, r24  \n\t"
        "pop   r24            \n\t"
        "reti                 \n\t"
        ::: "memory");
}
