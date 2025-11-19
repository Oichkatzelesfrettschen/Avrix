/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file ipv4.c
 * @brief IPv4 Protocol Implementation
 *
 * Minimal IPv4 stack with RFC 1071 compliant checksum and header validation.
 */

#include "ipv4.h"
#include "slip.h"
#include "drivers/tty/tty.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - CHECKSUM (RFC 1071 COMPLIANT)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Calculate Internet checksum (RFC 1071)
 *
 * NOVEL IMPROVEMENT: Proper carry propagation (original had bug).
 * Uses the standard one's complement sum algorithm with end-around carry.
 *
 * Algorithm:
 * 1. Sum all 16-bit words
 * 2. Add any leftover byte (odd length) as high-order byte
 * 3. Fold 32-bit sum to 16 bits by adding carry repeatedly
 * 4. Return one's complement
 */
uint16_t ipv4_checksum(const void *buf, size_t len) {
    const uint8_t *data = (const uint8_t *)buf;
    uint32_t sum = 0;

    /* Sum all 16-bit words */
    while (len > 1) {
        /* Network byte order: high byte first */
        sum += (uint16_t)((data[0] << 8) | data[1]);
        data += 2;
        len -= 2;
    }

    /* Add leftover byte (if odd length) as high-order byte */
    if (len > 0) {
        sum += (uint16_t)(data[0] << 8);
    }

    /* NOVEL: Proper carry folding (loop until no carry) */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* Return one's complement */
    return (uint16_t)~sum;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - HEADER MANIPULATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize IPv4 header with standard defaults
 */
void ipv4_init_header(ipv4_hdr_t *h, uint32_t src, uint32_t dst,
                      uint8_t proto, uint16_t payload_len) {
    /* Clear header to zeros */
    memset(h, 0, sizeof(*h));

    /* Set standard fields */
    h->ver_ihl = 0x45;  /* Version 4, IHL 5 (20 bytes) */
    h->tos     = 0x00;  /* Best effort */
    h->len     = ipv4_htons(sizeof(ipv4_hdr_t) + payload_len);
    h->id      = 0;     /* No fragmentation support */
    h->frag    = ipv4_htons(0x4000);  /* DF=1 (Don't Fragment) */
    h->ttl     = 64;    /* Standard default */
    h->proto   = proto;
    h->saddr   = ipv4_htonl(src);
    h->daddr   = ipv4_htonl(dst);

    /* Calculate and set checksum */
    h->checksum = 0;
    h->checksum = ipv4_checksum(h, sizeof(*h));
}

/**
 * @brief Validate IPv4 header
 *
 * NOVEL IMPROVEMENT: Header validation (original had none).
 */
bool ipv4_validate_header(const ipv4_hdr_t *h) {
    /* Check version (must be 4) */
    if ((h->ver_ihl >> 4) != 4) {
        return false;
    }

    /* Check IHL (must be 5 for 20-byte header, no options) */
    if ((h->ver_ihl & 0x0F) != 5) {
        return false;
    }

    /* Check total length (must be at least header size) */
    uint16_t total_len = ipv4_ntohs(h->len);
    if (total_len < sizeof(ipv4_hdr_t)) {
        return false;
    }

    /* Verify header checksum */
    uint16_t received_checksum = h->checksum;
    ipv4_hdr_t tmp;
    memcpy(&tmp, h, sizeof(tmp));
    tmp.checksum = 0;
    uint16_t calculated = ipv4_checksum(&tmp, sizeof(tmp));

    if (received_checksum != calculated) {
        return false;
    }

    return true;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - PACKET TRANSMISSION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Send IPv4 packet over SLIP/TTY
 *
 * NOVEL OPTIMIZATION: Direct transmission without intermediate buffer
 * (original copied header+payload to frame buffer first).
 *
 * However, SLIP requires contiguous buffer for encoding, so we still
 * need to assemble the packet. Future optimization: scatter-gather SLIP.
 */
void ipv4_send(tty_t *t, const ipv4_hdr_t *h, const void *payload, size_t len) {
    if (!t || !h) {
        return;  /* Invalid parameters */
    }

    /* Assemble packet: header + payload */
    uint8_t frame[sizeof(ipv4_hdr_t) + len];
    memcpy(frame, h, sizeof(ipv4_hdr_t));
    if (payload && len > 0) {
        memcpy(frame + sizeof(ipv4_hdr_t), payload, len);
    }

    /* Transmit via SLIP framing */
    slip_send_packet(t, frame, sizeof(frame));
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - PACKET RECEPTION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Receive IPv4 packet from SLIP/TTY
 *
 * NOVEL IMPROVEMENTS:
 * 1. Header validation (version, IHL, checksum)
 * 2. Configurable buffer size (IPV4_MTU instead of fixed 256)
 * 3. Proper error codes (0 vs -1)
 */
int ipv4_recv(tty_t *t, ipv4_hdr_t *h, void *payload, size_t len) {
    if (!t || !h) {
        return -1;  /* Invalid parameters */
    }

    /* Receive SLIP frame (header + payload) */
    uint8_t frame[IPV4_MTU];
    int frame_len = slip_recv_packet(t, frame, sizeof(frame));

    /* Check if we received enough data for header */
    if (frame_len < (int)sizeof(ipv4_hdr_t)) {
        return 0;  /* Incomplete or no packet */
    }

    /* Extract header */
    memcpy(h, frame, sizeof(ipv4_hdr_t));

    /* NOVEL: Validate header before processing payload */
    if (!ipv4_validate_header(h)) {
        return 0;  /* Invalid header (drop packet) */
    }

    /* Calculate payload length */
    int payload_len = frame_len - (int)sizeof(ipv4_hdr_t);
    if (payload_len < 0) {
        return 0;  /* Malformed packet */
    }

    /* Copy payload (truncate if buffer too small) */
    if (payload && len > 0) {
        size_t copy_len = (size_t)payload_len;
        if (copy_len > len) {
            copy_len = len;  /* Truncate to buffer size */
        }
        memcpy(payload, frame + sizeof(ipv4_hdr_t), copy_len);
    }

    return payload_len;
}
