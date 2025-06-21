#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*──────────────────────────────── tty.h ──────────────────────────────
 * Simple UART-style TTY with ring buffers.
 *   • Pure C23, no AVR dependencies
 *   • Not interrupt-driven – caller must poll the RX source.
 *   • TX writes use a user supplied callback.
 *
 * This minimal driver provides enough infrastructure for SLIP or
 * other serial protocols in tiny embedded environments.
 *────────────────────────────────────────────────────────────────────*/

typedef void (*tty_putc_fn)(uint8_t c);
typedef int  (*tty_getc_fn)(void);

typedef struct {
    uint8_t       *rx_buf;   /* RX ring buffer       */
    uint8_t       *tx_buf;   /* TX ring buffer       */
    uint8_t        rx_head;  /* RX head index        */
    uint8_t        rx_tail;  /* RX tail index        */
    uint8_t        tx_head;  /* TX head index        */
    uint8_t        tx_tail;  /* TX tail index        */
    uint8_t        size;     /* buffer size (power2) */
    tty_putc_fn    putc;     /* low-level TX output  */
    tty_getc_fn    getc;     /* low-level RX input   */
} tty_t;

/** Initialise a TTY instance.
 *  @param t      TTY descriptor.
 *  @param rx_buf RX buffer (size bytes).
 *  @param tx_buf TX buffer (size bytes).
 *  @param size   Capacity of each buffer (≤ 255).
 *  @param putc   Byte output callback (cannot be NULL).
 *  @param getc   Byte input callback. Return -1 when none.
 */
void tty_init(tty_t *t, uint8_t *rx_buf, uint8_t *tx_buf, uint8_t size,
              tty_putc_fn putc, tty_getc_fn getc);

/** Poll the RX source and enqueue available bytes. */
void tty_poll(tty_t *t);

/** Read up to len bytes from the RX buffer.
 *  @return number of bytes read.
 */
int tty_read(tty_t *t, uint8_t *dst, size_t len);

/** Write len bytes to TX, flushing immediately via putc.
 *  @return number of bytes written.
 */
int tty_write(tty_t *t, const uint8_t *src, size_t len);

/** Bytes currently available in the RX buffer. */
size_t tty_rx_available(const tty_t *t);

#ifdef __cplusplus
}
#endif

#endif /* TTY_H */
