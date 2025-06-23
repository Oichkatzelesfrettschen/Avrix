/* SPDX-License-Identifier: MIT */
/* Optional AVR GDB stub. Tiny blocking implementation used for
 * QEMU or on-device debugging. Built only when DEBUG_GDB is set. */

#include "gdbstub.h"

#ifdef DEBUG_GDB
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
#  define F_CPU 16000000UL
#endif
#define BAUD 115200
#define UBRR_VAL ((F_CPU / 16 / BAUD) - 1)

static void uart_init(void)
{
    UBRR0H = (uint8_t)(UBRR_VAL >> 8);
    UBRR0L = (uint8_t)UBRR_VAL;
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

static void uart_put(uint8_t c)
{
    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = c;
}

static uint8_t uart_get(void)
{
    while (!(UCSR0A & _BV(RXC0)));
    return UDR0;
}

void gdbstub_init(void)
{
    uart_init();
}

void gdbstub_break(void)
{
    asm volatile("break");
}

#else

void gdbstub_init(void) {}
void gdbstub_break(void) {}

#endif /* DEBUG_GDB */

