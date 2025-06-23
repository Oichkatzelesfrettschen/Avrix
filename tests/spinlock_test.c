#include "nk_superlock.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>

/* Global lock shared between main context and the timer ISR. */
static volatile nk_spinlock_t g_lock = NK_SPINLOCK_STATIC_INIT;
static volatile uint16_t   g_ticks;

/* 1 kHz timer interrupt acquires and releases the lock. */
ISR(TIMER0_COMPA_vect)
{
    nk_spinlock_lock_rt((nk_spinlock_t*)&g_lock);
    nk_spinlock_unlock_rt((nk_spinlock_t*)&g_lock);
    ++g_ticks;
}

int main(void)
{
    nk_spinlock_init((nk_spinlock_t*)&g_lock);

    /* Configure Timer0 in CTC mode, prescaler = 64. */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);
    OCR0A  = (uint8_t)((F_CPU / 64UL / 1000UL) - 1);
    TIMSK0 = _BV(OCIE0A);
    sei();

    /* Exercise the lock in parallel with the ISR. */
    for (unsigned i = 0; i < 5000; ++i) {
        nk_spinlock_lock_rt((nk_spinlock_t*)&g_lock);
        nk_spinlock_unlock_rt((nk_spinlock_t*)&g_lock);
    }

    cli();
    printf("ticks:%u\n", g_ticks);
    return g_ticks > 0 ? 0 : 1;
}
