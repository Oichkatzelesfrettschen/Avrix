/* SPDX-License-Identifier: MIT */
#include "avr_gdbstub.h"
#include <stddef.h>

static tty_t *gdb_tty;

static void put(uint8_t c) {
    if (gdb_tty && gdb_tty->putc)
        gdb_tty->putc(c);
}

static int get(void) {
    return (gdb_tty && gdb_tty->getc) ? gdb_tty->getc() : -1;
}

static void hex(uint8_t b) {
    const char h[] = "0123456789abcdef";
    put(h[(b >> 4) & 0xF]);
    put(h[b & 0xF]);
}

static void packet(const char *p) {
    uint8_t sum = 0;
    put('$');
    for (; p && *p; ++p) {
        put((uint8_t)*p);
        sum += (uint8_t)*p;
    }
    put('#');
    hex(sum);
    while (get() != '+') {}
}

void gdbstub_init(tty_t *tty) {
    gdb_tty = tty;
    packet("OK");
}

void gdbstub_poll(void) {
    if (!gdb_tty) return;
    int c;
    while ((c = get()) >= 0) {
        if (c == 0x03) {         /* Ctrl-C */
            asm volatile("break");
            packet("S05");
        } else if (c == '?') {
            packet("S05");
        } else {
            packet("");          /* nack unsupported */
        }
    }
}

void gdbstub_break(void) {
    asm volatile("break");
}

