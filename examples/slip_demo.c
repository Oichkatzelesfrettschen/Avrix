#include "ipv4.h"
#include "slip.h"
#include "tty.h"
#include <stdio.h>
#include <string.h>

static void host_putc(uint8_t c)
{
    putchar(c);
}

static int host_getc(void)
{
    return -1; /* no input in demo */
}

int main(void)
{
    uint8_t rx[64];
    uint8_t tx[64];
    tty_t tty;
    tty_init(&tty, rx, tx, sizeof rx, host_putc, host_getc);

    const char msg[] = "SLIP demo";
    ipv4_hdr_t h;
    ipv4_init_header(&h, 0x0a000001u, 0x0a000002u, 0x11, sizeof msg);
    ipv4_send(&tty, &h, (const uint8_t *)msg, sizeof msg);

    return 0;
}
