/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file slip.c
 * @brief SLIP Protocol Implementation
 *
 * RFC 1055 compliant encoder/decoder for Serial Line IP.
 * Stateless design suitable for embedded systems.
 */

#include "slip.h"
#include "drivers/tty/tty.h"
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - SLIP ENCODING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Encode and transmit a SLIP frame
 *
 * Implements RFC 1055 character stuffing algorithm.
 */
void slip_send_packet(tty_t *t, const uint8_t *buf, size_t len) {
    if (!t) {
        return;  /* Invalid TTY */
    }

    /* Send frame start delimiter */
    const uint8_t start = SLIP_END;
    tty_write(t, &start, 1);

    /* Encode and transmit payload with character stuffing */
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = buf[i];

        if (b == SLIP_END) {
            /* Encode END as: ESC ESC_END */
            const uint8_t esc_seq[2] = {SLIP_ESC, SLIP_ESC_END};
            tty_write(t, esc_seq, 2);
        }
        else if (b == SLIP_ESC) {
            /* Encode ESC as: ESC ESC_ESC */
            const uint8_t esc_seq[2] = {SLIP_ESC, SLIP_ESC_ESC};
            tty_write(t, esc_seq, 2);
        }
        else {
            /* Regular byte - no encoding needed */
            tty_write(t, &b, 1);
        }
    }

    /* Send frame end delimiter */
    const uint8_t end = SLIP_END;
    tty_write(t, &end, 1);
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - SLIP DECODING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Decode a SLIP frame from TTY RX buffer
 *
 * Reads bytes from TTY until a complete frame is received.
 * Handles escape sequence decoding per RFC 1055.
 *
 * @return Number of bytes in decoded frame, 0 if incomplete
 */
int slip_recv_packet(tty_t *t, uint8_t *buf, size_t len) {
    if (!t || !buf || len == 0) {
        return 0;  /* Invalid parameters */
    }

    size_t pos = 0;          /* Position in output buffer */
    bool esc = false;        /* Escape sequence state */
    uint8_t byte;

    /* Read bytes from TTY until frame complete or no more data */
    while (tty_rx_available(t) > 0) {
        /* Read one byte from TTY */
        int nread = tty_read(t, &byte, 1);
        if (nread <= 0) {
            break;  /* No more data available */
        }

        /* Handle escape sequence decoding */
        if (esc) {
            if (byte == SLIP_ESC_END) {
                byte = SLIP_END;  /* ESC ESC_END → END */
            }
            else if (byte == SLIP_ESC_ESC) {
                byte = SLIP_ESC;  /* ESC ESC_ESC → ESC */
            }
            else {
                /* Invalid escape sequence - discard and continue */
                esc = false;
                continue;
            }
            esc = false;
        }
        else {
            /* Check for special characters */
            if (byte == SLIP_END) {
                /* Frame delimiter detected */
                if (pos > 0) {
                    /* Complete frame received - return it */
                    return (int)pos;
                }
                /* Empty frame or leading delimiter - continue */
                continue;
            }

            if (byte == SLIP_ESC) {
                /* Start of escape sequence */
                esc = true;
                continue;
            }
        }

        /* Add decoded byte to buffer if space available */
        if (pos < len) {
            buf[pos++] = byte;
        }
        else {
            /* Buffer full - discard frame (too large) */
            /* Continue reading until END to synchronize */
            while (tty_rx_available(t) > 0) {
                if (tty_read(t, &byte, 1) > 0 && byte == SLIP_END) {
                    break;
                }
            }
            return 0;  /* Frame too large */
        }
    }

    /* No complete frame available yet */
    return 0;
}
