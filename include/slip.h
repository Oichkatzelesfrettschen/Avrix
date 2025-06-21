#ifndef SLIP_H
#define SLIP_H

#include <stdint.h>
#include <stddef.h>
#include "tty.h"

#ifdef __cplusplus
extern "C" {
#endif

/*──────────────────────────────── slip.h ─────────────────────────────
 * SLIP (RFC1055) framing over a generic TTY.
 *   • stateless encoder / decoder
 *   • suitable for tiny microcontrollers
 *────────────────────────────────────────────────────────────────────*/

#define SLIP_END      0xC0u
#define SLIP_ESC      0xDBu
#define SLIP_ESC_END  0xDCu
#define SLIP_ESC_ESC  0xDDu

/** Encode and transmit a SLIP frame. */
void slip_send_packet(tty_t *t, const uint8_t *buf, size_t len);

/**
 * Decode a SLIP frame from the TTY RX buffer.
 * @param t    TTY descriptor
 * @param buf  Destination buffer
 * @param len  Capacity of destination buffer
 * @return     Number of bytes decoded, 0 if no complete frame.
 */
int slip_recv_packet(tty_t *t, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SLIP_H */
