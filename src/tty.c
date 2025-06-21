/*────────────────────────────── tty.c ─────────────────────────────
   Minimal ring-buffer TTY for µ-UNIX and host demos
   -------------------------------------------------
   • Pure C23, no AVR dependencies
   • Polling based RX, immediate TX via callback
   • Suitable foundation for SLIP or shell interfaces
   -------------------------------------------------*/

#include "tty.h"

void tty_init(tty_t *t, uint8_t *rx_buf, uint8_t *tx_buf, uint8_t size,
              tty_putc_fn putc, tty_getc_fn getc)
{
    t->rx_buf  = rx_buf;
    t->tx_buf  = tx_buf;
    t->size    = size;
    t->rx_head = t->rx_tail = 0;
    t->tx_head = t->tx_tail = 0;
    t->putc    = putc;
    t->getc    = getc;
}

void tty_poll(tty_t *t)
{
    if (!t->getc) return;
    int c;
    while ((c = t->getc()) >= 0) {
        uint8_t next = (uint8_t)(t->rx_head + 1) % t->size;
        if (next == t->rx_tail) {
            break; /* overflow */
        }
        t->rx_buf[t->rx_head] = (uint8_t)c;
        t->rx_head = next;
    }
}

static int tty_buf_read(uint8_t *buf, uint8_t *head, uint8_t *tail,
                        uint8_t size, uint8_t *dst, size_t len)
{
    size_t i = 0;
    while (i < len && *tail != *head) {
        dst[i++] = buf[*tail];
        *tail = (uint8_t)(*tail + 1) % size;
    }
    return (int)i;
}

static int tty_buf_write(uint8_t *buf, uint8_t *head, uint8_t *tail,
                         uint8_t size, const uint8_t *src, size_t len)
{
    size_t i = 0;
    while (i < len) {
        uint8_t next = (uint8_t)(*head + 1) % size;
        if (next == *tail) break; /* full */
        buf[*head] = src[i++];
        *head = next;
    }
    return (int)i;
}

int tty_read(tty_t *t, uint8_t *dst, size_t len)
{
    return tty_buf_read(t->rx_buf, &t->rx_head, &t->rx_tail, t->size, dst, len);
}

int tty_write(tty_t *t, const uint8_t *src, size_t len)
{
    int wrote = tty_buf_write(t->tx_buf, &t->tx_head, &t->tx_tail,
                              t->size, src, len);
    for (int i = 0; i < wrote; ++i) {
        if (t->putc)
            t->putc(t->tx_buf[t->tx_tail]);
        t->tx_tail = (uint8_t)(t->tx_tail + 1) % t->size;
    }
    return wrote;
}

size_t tty_rx_available(const tty_t *t)
{
    return (t->rx_head - t->rx_tail + t->size) % t->size;
}
