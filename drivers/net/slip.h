/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file slip.h
 * @brief SLIP (Serial Line IP) Protocol Implementation
 *
 * RFC 1055 compliant SLIP framing for packet transmission over serial links.
 * Designed for embedded systems with minimal memory footprint.
 *
 * ## Overview
 * SLIP provides a simple packet framing mechanism for transmitting IP packets
 * over serial lines (RS-232, UART, etc.). It uses two special characters:
 * - END (0xC0): Frame delimiter
 * - ESC (0xDB): Escape character for encoding END/ESC in data
 *
 * ## Features
 * - Stateless encoder/decoder (no persistent state between packets)
 * - Zero-copy encoding where possible
 * - Suitable for tiny microcontrollers (8-bit AVR, Cortex-M0, etc.)
 * - Works with any TTY abstraction (UART, USB-CDC, etc.)
 *
 * ## Protocol
 * RFC 1055 defines the encoding:
 * - Frame starts and ends with END (0xC0)
 * - END in data → ESC ESC_END (0xDB 0xDC)
 * - ESC in data → ESC ESC_ESC (0xDB 0xDD)
 *
 * ## Memory Footprint
 * - Flash: ~150 bytes (encoder + decoder)
 * - RAM: 0 bytes persistent state (stateless)
 * - Stack: ~10 bytes during operations
 *
 * ## Usage
 * ```c
 * tty_t serial;
 * tty_init(&serial, ...);
 *
 * // Send packet
 * uint8_t tx_data[] = {0x45, 0x00, 0x00, 0x54, ...};  // IP packet
 * slip_send_packet(&serial, tx_data, sizeof(tx_data));
 *
 * // Receive packet
 * uint8_t rx_buf[1500];  // MTU size
 * int len = slip_recv_packet(&serial, rx_buf, sizeof(rx_buf));
 * if (len > 0) {
 *     // Process received packet
 * }
 * ```
 *
 * ## Limitations
 * - No compression (see RFC 1144 for CSLIP)
 * - No error detection (use higher-layer checksums)
 * - No flow control (handled by serial hardware)
 */

#ifndef DRIVERS_NET_SLIP_H
#define DRIVERS_NET_SLIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*═══════════════════════════════════════════════════════════════════
 * SLIP PROTOCOL CONSTANTS (RFC 1055)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Frame delimiter character
 *
 * Marks the beginning and end of a SLIP frame.
 * Value: 0xC0 (192 decimal)
 */
#define SLIP_END      0xC0u

/**
 * @brief Escape character
 *
 * Used to encode END and ESC characters in data payload.
 * Value: 0xDB (219 decimal)
 */
#define SLIP_ESC      0xDBu

/**
 * @brief Escaped END character
 *
 * Transmitted as: ESC ESC_END (0xDB 0xDC) to represent END in data.
 * Value: 0xDC (220 decimal)
 */
#define SLIP_ESC_END  0xDCu

/**
 * @brief Escaped ESC character
 *
 * Transmitted as: ESC ESC_ESC (0xDB 0xDD) to represent ESC in data.
 * Value: 0xDD (221 decimal)
 */
#define SLIP_ESC_ESC  0xDDu

/*═══════════════════════════════════════════════════════════════════
 * FORWARD DECLARATIONS
 *═══════════════════════════════════════════════════════════════════*/

/* TTY type forward declaration (defined in drivers/tty/tty.h) */
typedef struct tty_s tty_t;

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - SLIP ENCODING/DECODING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Encode and transmit a SLIP frame
 *
 * Encodes the buffer according to RFC 1055 and transmits via TTY:
 * 1. Send END character (frame start)
 * 2. For each byte in buffer:
 *    - If byte == END: send ESC ESC_END
 *    - If byte == ESC: send ESC ESC_ESC
 *    - Otherwise: send byte as-is
 * 3. Send END character (frame end)
 *
 * @param t TTY descriptor (serial port)
 * @param buf Data buffer to encode and transmit
 * @param len Length of data buffer in bytes
 *
 * @note This function is stateless and can be called at any time
 * @note Transmission is immediate (no buffering beyond TTY layer)
 * @note If len == 0, sends empty frame (END END)
 *
 * Example:
 * ```c
 * uint8_t packet[] = {0x45, 0x00, 0x00, 0x1C};  // IP header fragment
 * slip_send_packet(&uart, packet, sizeof(packet));
 * ```
 */
void slip_send_packet(tty_t *t, const uint8_t *buf, size_t len);

/**
 * @brief Decode a SLIP frame from TTY RX buffer
 *
 * Attempts to read and decode one complete SLIP frame from the TTY.
 * Returns immediately if no complete frame is available.
 *
 * Decoding algorithm:
 * 1. Read bytes from TTY until END character
 * 2. Decode escape sequences:
 *    - ESC ESC_END → END
 *    - ESC ESC_ESC → ESC
 * 3. Return decoded frame length
 *
 * @param t TTY descriptor (serial port)
 * @param buf Destination buffer for decoded frame
 * @param len Capacity of destination buffer (bytes)
 * @return Number of bytes decoded, 0 if no complete frame available
 *
 * @note Returns 0 if no complete frame is ready (non-blocking)
 * @note Discards incomplete frames if buffer is too small
 * @note Leading END characters (frame delimiters) are skipped
 * @note This function is stateless (no persistent decoder state)
 *
 * Example:
 * ```c
 * uint8_t rx_buf[1500];
 * while (1) {
 *     tty_poll(&uart);  // Poll for incoming data
 *     int len = slip_recv_packet(&uart, rx_buf, sizeof(rx_buf));
 *     if (len > 0) {
 *         // Process complete packet
 *         handle_ip_packet(rx_buf, len);
 *     }
 * }
 * ```
 */
int slip_recv_packet(tty_t *t, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_NET_SLIP_H */
