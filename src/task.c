/*──────────────────────── nk_task.c ────────────────────────────
   µ-UNIX round-robin scheduler + 1 kHz tick   |   ATmega328P
   Foot-print  ≈ 640 B flash / 70 B SRAM

   build: avr-gcc 14  -std=gnu2b -Oz -flto -mrelax -mmcu=atmega328p
  ───────────────────────────────────────────────────────────────*/
#include "nk_task.h"
#include "door.h"
#include <string.h>
#include <avr/interrupt.h>

/*──────── kernel globals ──────────────────────────────────────*/
static nk_tcb_t *tcb_pool[MAX_TASKS];
static uint8_t   nk_tasks  = 0;
static uint8_t   nk_cur    = 0;          /* exported via nk_cur_tid() */

/* Door-descriptor matrix in .noinit */
door_t door_vec[MAX_TASKS][DOOR_SLOTS]
        __attribute__((section(".noinit")));

/* quantum counter (1 ms tick, 10 ms slice) */
#ifndef NK_QUANTUM_TICKS
#  define NK_QUANTUM_TICKS 10
#endif
static volatile uint8_t tick_left = NK_QUANTUM_TICKS;

/*──────── low-level context switch (asm in isr.S) ────────────*/
extern void _nk_context_swap(nk_sp_t *save, nk_sp_t load);

/*———— fast helpers ————————————————————————————————*/
uint8_t nk_cur_tid(void)                         { return nk_cur; }

/* cooperative yield – 6 cycles + soft tick */
void nk_yield(void) __attribute__((naked));
void nk_yield(void)
{
    asm volatile(
        "push r18            \n\t"
        "lds  r18, tick_left \n\t"
        "dec  r18            \n\t"
        "sts  tick_left, r18 \n\t"
        "brne 1f             \n\t"       /* slice not expired */
        "call nk_sched_tick  \n"
        "1: pop r18          \n\t"
        "ret                 \n");
}

/*————- scheduler initialisation ————————————————*/
void nk_sched_init(void)
{
    nk_tasks = nk_cur = 0;
    memset(door_vec, 0, sizeof door_vec);

    /* TIMER0 CTC → 1 kHz */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01)|_BV(CS00);        /* clk/64 */
    OCR0A  = 250-1;
    TIMSK0 = _BV(OCIE0A);
}

/*————- add task ————————————————————————————————*/
void nk_task_add(nk_tcb_t *t,
                 void (*entry)(void),
                 void *stack_top,
                 uint8_t prio, uint8_t class)
{
    if (nk_tasks >= MAX_TASKS) return;

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

/*————- next-task picker (priority then round-robin) ————*/
static uint8_t pick_next(void)
{
    uint8_t best = nk_cur;
    uint8_t best_key = 0xFF;
    for(uint8_t i = 0; i < nk_tasks; ++i) {
        uint8_t idx = (uint8_t)((nk_cur + 1 + i) % nk_tasks);
        nk_tcb_t *t = tcb_pool[idx];
#if NK_OPT_DAG_WAIT
        if(t->state == NK_READY && t->deps == 0 && t->prio < best_key)
#else
        if(t->state == NK_READY && t->prio < best_key)
#endif
        { best = idx; best_key = t->prio; }
    }
    return best;
}

/*————- tick handler (called from ISR & yield) ———————*/
void nk_sched_tick(void)
{
    uint8_t next = pick_next();
    if(next == nk_cur) return;

    nk_tcb_t *from = tcb_pool[nk_cur];
    nk_tcb_t *to   = tcb_pool[next];
    from->state = NK_READY;
    to->state   = NK_RUNNING;
    nk_cur      = next;
    _nk_context_swap(&from->sp, to->sp);
}

/*————- 1 kHz timer ISR ———————————————————————————*/
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    asm volatile(
        "push r18              \n\t"
        "lds  r18, tick_left   \n\t"
        "dec  r18              \n\t"
        "sts  tick_left, r18   \n\t"
        "brne 1f               \n\t"
        "ldi  r18, %0          \n\t"
        "sts  tick_left, r18   \n\t"
        "call nk_sched_tick    \n"
        "1: pop  r18           \n\t"
        "reti                  "
        :: "M"(NK_QUANTUM_TICKS));
}

/*————- run scheduler (never returns) ———————————*/
void nk_sched_run(void)
{
    sei();
    for(;;) asm volatile("sleep");
}

/*──────── optional DAG wait/signal ——————————————*/
#if NK_OPT_DAG_WAIT
void nk_task_block(nk_tcb_t *t, uint8_t deps)
{
    t->deps  = deps;
    t->state = NK_BLOCKED;
}
void nk_task_signal(nk_tcb_t *t)
{
    if(t->deps && --t->deps == 0) t->state = NK_READY;
}
#endif

/*──────── fallback stub if ASM swap absent ———————*/
__attribute__((weak,naked))
void _nk_context_swap(nk_sp_t *save, nk_sp_t load)
{
    asm volatile("ret");
}
