/*──────────────────────── nk_task.c ────────────────────────────
   µ-UNIX round-robin scheduler + 1 kHz tick   |   ATmega328P
   Foot-print  ≈ 640 B flash / 70 B SRAM

   build: avr-gcc 14  -std=gnu2b -Oz -flto -mrelax -mmcu=atmega328p
   -----------------------------------------------------------------*/

#include "nk_task.h"
#include "door.h"                 /* door_t matrix lives in .noinit   */

#if NK_OPT_STACK_GUARD            /* optional stack sentinels */
#  include "memguard.h"
#endif

#include <string.h>
#include <avr/interrupt.h>

/*──────────────────────── 1.  Tunables ─────────────────────────*/

#ifndef NK_QUANTUM_TICKS          /* scheduler slice = n × 1 ms ticks */
#  define NK_QUANTUM_TICKS  10
#endif

/*──────────────────────── 2.  Globals (.bss / .noinit) ─────────*/

static nk_tcb_t *tcb_pool[NK_MAX_TASKS];
static uint8_t   nk_tasks     = 0;
static uint8_t   nk_cur       = 0;        /* exported via nk_cur_tid() */

door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS]        /* IPC slabs         */
        __attribute__((section(".noinit")));

static volatile uint8_t tick_left = NK_QUANTUM_TICKS;

#if NK_OPT_STACK_GUARD
/* each stack = GUARD | payload | GUARD  (defined in memguard.h)      */
static uint8_t nk_stacks[NK_MAX_TASKS][NK_STACK_SIZE + 2*GUARD_BYTES];
#endif

/*────────── low-level asm context swap (provided in isr.S) ──────────*/
extern void _nk_context_swap(nk_sp_t *save, nk_sp_t load);

/*──────────────────────── 3.  Tiny helpers ─────────────────────*/

uint8_t nk_cur_tid(void)                     { return nk_cur; }

/* cooperative yield – 6 cycles + soft tick */
__attribute__((naked))
void nk_yield(void)
{
    asm volatile(
        "push r18              \n\t"
        "lds  r18, tick_left   \n\t"
        "dec  r18              \n\t"
        "sts  tick_left, r18   \n\t"
        "brne 1f               \n\t"   /* slice not expired */
        "call nk_sched_tick    \n"
        "1: pop  r18           \n\t"
        "ret                   \n");
}

/*──────────────────────── 4.  Init & add task ─────────────────*/

void nk_sched_init(void)
{
    nk_tasks = nk_cur = 0;
    memset(door_vec, 0, sizeof door_vec);

    /* TIMER0 CTC → 1 kHz  (16 MHz / 64 / 250) */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);      /* clk/64 prescale */
    OCR0A  = 250 - 1;
    TIMSK0 = _BV(OCIE0A);
}

void nk_task_add(nk_tcb_t *t,
                 void (*entry)(void),
                 void *stack_top,
                 uint8_t prio,
                 uint8_t class)
{
    if (nk_tasks >= NK_MAX_TASKS) return;

#if NK_OPT_STACK_GUARD
    /* internal stack pool + sentinels */
    uint8_t *region = nk_stacks[nk_tasks];
    guard_init(region, sizeof nk_stacks[0]);

    stack_top = region + GUARD_BYTES + NK_STACK_SIZE;
#endif

    /* push fake return addr (entry) so `reti` jumps into task */
    uint8_t *sp = (uint8_t *)stack_top;
    *--sp = (uint16_t)entry & 0xFF;
    *--sp = (uint16_t)entry >> 8;

    t->sp    = (nk_sp_t)sp;
    t->state = NK_READY;
    t->prio  = (class << 6) | (prio & 0x3F);
    t->pid   = nk_tasks;
#if NK_OPT_DAG_WAIT
    t->deps  = 0;
#endif
    tcb_pool[nk_tasks++] = t;
}

/*──────────────────────── 5.  Pick-next logic ─────────────────*/

static uint8_t pick_next(void)
{
    uint8_t best      = nk_cur;
    uint8_t best_key  = 0xFF;

    for (uint8_t i = 0; i < nk_tasks; ++i) {
        uint8_t idx = (uint8_t)((nk_cur + 1 + i) % nk_tasks);
        nk_tcb_t *t = tcb_pool[idx];

        bool ready = (t->state == NK_READY);
#if NK_OPT_DAG_WAIT
        ready &= (t->deps == 0);
#endif
        if (ready && t->prio < best_key) {
            best      = idx;
            best_key  = t->prio;
        }
    }
    return best;
}

/*──────────────────────── 6.  Tick / context switch ───────────*/

void nk_sched_tick(void)
{
    uint8_t next = pick_next();
    if (next == nk_cur) return;

    nk_tcb_t *from = tcb_pool[nk_cur];
    nk_tcb_t *to   = tcb_pool[next];

    from->state = NK_READY;
    to->state   = NK_RUNNING;
    nk_cur      = next;

    _nk_context_swap(&from->sp, to->sp);   /* never returns here */
}

/* 1 kHz Timer-0 ISR – decrements quantum and calls scheduler */
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    asm volatile(
        "push r18                \n\t"
        "lds  r18, tick_left     \n\t"
        "dec  r18                \n\t"
        "sts  tick_left, r18     \n\t"
        "brne 1f                 \n\t"
        "ldi  r18, %0            \n\t"
        "sts  tick_left, r18     \n\t"
        "call nk_sched_tick      \n"
        "1: pop  r18             \n\t"
        "reti                    "
        :: "M"(NK_QUANTUM_TICKS));
}

/*──────────────────────── 7.  Run-forever loop ───────────────*/

void nk_sched_run(void)  __attribute__((noreturn));
void nk_sched_run(void)
{
    sei();                           /* global IRQ enable */
    for (;;) asm volatile("sleep");  /* idle task = MCU sleep */
}

/*──────────────────────── 8.  DAG helpers (optional) ─────────*/

#if NK_OPT_DAG_WAIT
void nk_task_block(nk_tcb_t *t, uint8_t deps)
{
    t->deps  = deps;
    t->state = NK_BLOCKED;
}

void nk_task_signal(nk_tcb_t *t)
{
    if (t->deps && --t->deps == 0)
        t->state = NK_READY;
}
#endif

/*──────────────────────── 9.  Fallback context swap ──────────*/
/* in case isr.S not linked yet – keeps linker happy for tests */
__attribute__((weak,naked))
void _nk_context_swap(nk_sp_t *save, nk_sp_t load)
{
    asm volatile("ret");
}
