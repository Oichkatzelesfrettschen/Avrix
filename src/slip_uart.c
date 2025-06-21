/*──────────────────────────── slip_uart.c ───────────────────────────
   SLIP (RFC1055) encoder/decoder over a generic TTY
   -------------------------------------------------
   • Stateless framing for tiny microcontrollers
   • Suitable for WiFi modules or network hats
   -------------------------------------------------*/

#include "slip.h"

void slip_send_packet(tty_t *t, const uint8_t *buf, size_t len)
{
    if (!t) return;
    tty_write(t, (const uint8_t[]){SLIP_END}, 1);
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = buf[i];
        if (b == SLIP_END) {
            uint8_t esc[] = {SLIP_ESC, SLIP_ESC_END};
            tty_write(t, esc, 2);
        } else if (b == SLIP_ESC) {
            uint8_t esc[] = {SLIP_ESC, SLIP_ESC_ESC};
            tty_write(t, esc, 2);
        } else {
            tty_write(t, &b, 1);
        }
    }
    tty_write(t, (const uint8_t[]){SLIP_END}, 1);
}

int slip_recv_packet(tty_t *t, uint8_t *buf, size_t len)
{
    size_t pos = 0;
    bool esc = false;
    int c;
    while ((c = tty_read(t, &buf[pos], 1)) > 0 || tty_rx_available(t)) {
        if (c <= 0) {
            uint8_t tmp;
            if (tty_read(t, &tmp, 1) <= 0) break;
            c = tmp;
        }
        uint8_t b = (uint8_t)c;
        if (esc) {
            if (b == SLIP_ESC_END)      b = SLIP_END;
            else if (b == SLIP_ESC_ESC) b = SLIP_ESC;
            else {
                esc = false;
                continue;
            }
            esc = false;
        } else {
            if (b == SLIP_END) {
                if (pos > 0) return (int)pos;
                else continue;
            }
            if (b == SLIP_ESC) {
                esc = true;
                continue;
            }
        }
        if (pos < len) buf[pos++] = b;
    }
    return 0;
}
