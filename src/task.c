/*──────────────────────── nk_task.c ───────────────────────────
   Tiny round-robin scheduler for µ-UNIX on ATmega328P.
   - 8-cycle context switch in asm (nk_switch_to)
   - fits 64-byte TCB pool + 1-byte current-tid
   - initialise door-vector table (in .noinit)

   build: gcc-14 (AVR)  -std=gnu2b -Oz -flto -mrelax
  ─────────────────────────────────────────────────────────────*/
#include "nk_task.h"
#include "door.h"
#include "kalloc.h"
#include <string.h>
#include <avr/interrupt.h>

/*──────── persistent kernel state ───────*/
static nk_tcb_t *tcb_pool[MAX_TASKS];
static uint8_t   nk_task_cnt  = 0;
static uint8_t   nk_cur       = 0;        /* exported via nk_cur_tid() */

/* door-descriptor matrix (.noinit so soft reset keeps mappings) */
door_t door_vec[MAX_TASKS][DOOR_SLOTS]
        __attribute__((section(".noinit")));

/*──── low-level asm hooks (isr.S) ───────*/
extern void nk_switch_to(uint8_t tid);    /* pre-empt / directed switch */

/*──────── API impl ──────────────────────*/
uint8_t nk_cur_tid(void) { return nk_cur; }

    
/*------------ scheduler init ------------*/
void nk_sched_init(void)
{
    nk_task_cnt = 0;
    nk_cur      = 0;
    memset(door_vec, 0, sizeof door_vec);   /* .noinit needs cold-init */
    kalloc_init();
}

/*------------ add task ------------------*/
void nk_task_add(nk_tcb_t *t,
                 void (*entry)(void),
                 void *stack_top,
                 uint8_t prio, uint8_t class)
{
    if(nk_task_cnt >= MAX_TASKS) return;

    uint8_t *sp = (uint8_t *)stack_top;

    /* fabricate initial frame : return → entry */
    *--sp = (uint16_t)entry & 0xFF;
    *--sp = (uint16_t)entry >> 8;

    t->sp     = (nk_sp_t)sp;
    t->state  = NK_READY;
    t->prio   = (class << 6) | (prio & 0x3F);
    t->pid    = nk_task_cnt;

    tcb_pool[nk_task_cnt++] = t;
}

/*------------ round-robin run loop ------*/
void nk_sched_run(void) __attribute__((noreturn));
void nk_sched_run(void)
{
    if(!nk_task_cnt) for(;;);               /* no tasks → hang */

    sei();                                  /* enable IRQs now */

    while(1) {
        uint8_t next = nk_cur;
        /* naive round-robin; skip blocked tasks */
        do {
            next = (uint8_t)((next + 1U) % nk_task_cnt);
        } while(tcb_pool[next]->state == NK_BLOCKED);

        if(next != nk_cur) {
            nk_switch_to(next);             /* ASM does save/restore */
            /* control returns here in old task after switch-back */
        }
        /* tick ISR will bump nk_cur when quantum expires        */
    }
}

/*----- cooperative / directed switch (used by Doors, locks) ---*/
void nk_switch_to(uint8_t tid) __attribute__((weak)); /* satisfy linker */
void nk_switch_to(uint8_t tid) { (void)tid; }         /* if ASM missing */
