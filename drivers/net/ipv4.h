/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file ipv4.h
 * @brief Minimal IPv4 Protocol Implementation
 */

#ifndef DRIVERS_NET_IPV4_H
#define DRIVERS_NET_IPV4_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include "avrix-config.h"

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

#ifndef IPV4_MTU
#  define IPV4_MTU 576
#endif

/*═══════════════════════════════════════════════════════════════════
 * IPv4 HEADER STRUCTURE (RFC 791)
 *═══════════════════════════════════════════════════════════════════*/

typedef struct {
    uint8_t  ver_ihl;    /**< Version (4) + IHL (5) = 0x45 */
    uint8_t  tos;        /**< Type of Service (DSCP + ECN) */
    uint16_t len;        /**< Total length (header + payload) */
    uint16_t id;         /**< Identification (for fragmentation) */
    uint16_t frag;       /**< Flags (3 bits) + Fragment offset (13 bits) */
    uint8_t  ttl;        /**< Time To Live (hop count) */
    uint8_t  proto;      /**< Protocol (TCP=6, UDP=17, etc.) */
    uint16_t checksum;   /**< Header checksum */
    uint32_t saddr;      /**< Source IP address */
    uint32_t daddr;      /**< Destination IP address */
} __attribute__((packed)) ipv4_hdr_t;

/*═══════════════════════════════════════════════════════════════════
 * IPv4 PROTOCOL NUMBERS (IANA)
 *═══════════════════════════════════════════════════════════════════*/

#define IPV4_PROTO_ICMP   1
#define IPV4_PROTO_TCP    6
#define IPV4_PROTO_UDP   17

/*═══════════════════════════════════════════════════════════════════
 * ENDIANNESS CONVERSION (Always Available)
 *═══════════════════════════════════════════════════════════════════*/

static inline uint16_t ipv4_htons(uint16_t x) {
    return (uint16_t)((x >> 8) | (x << 8));
}

static inline uint32_t ipv4_htonl(uint32_t x) {
    return ((x >> 24) & 0x000000FFu) |
           ((x >>  8) & 0x0000FF00u) |
           ((x <<  8) & 0x00FF0000u) |
           ((x << 24) & 0xFF000000u);
}

#define ipv4_ntohs ipv4_htons
#define ipv4_ntohl ipv4_htonl

/*═══════════════════════════════════════════════════════════════════
 * FORWARD DECLARATIONS
 *═══════════════════════════════════════════════════════════════════*/

typedef struct tty_s tty_t;

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API
 *═══════════════════════════════════════════════════════════════════*/

#if CONFIG_NET_IPV4_ENABLED

uint16_t ipv4_checksum(const void *buf, size_t len);
void ipv4_init_header(ipv4_hdr_t *h, uint32_t src, uint32_t dst,
                      uint8_t proto, uint16_t payload_len);
bool ipv4_validate_header(const ipv4_hdr_t *h);
void ipv4_send(tty_t *t, const ipv4_hdr_t *h, const void *payload, size_t len);
int ipv4_recv(tty_t *t, ipv4_hdr_t *h, void *payload, size_t len);

#else /* Stubs */

static inline uint16_t ipv4_checksum(const void *buf, size_t len) {
    (void)buf; (void)len; return 0;
}
static inline void ipv4_init_header(ipv4_hdr_t *h, uint32_t src, uint32_t dst,
                                    uint8_t proto, uint16_t payload_len) {
    (void)h; (void)src; (void)dst; (void)proto; (void)payload_len;
}
static inline bool ipv4_validate_header(const ipv4_hdr_t *h) {
    (void)h; return false;
}
static inline void ipv4_send(tty_t *t, const ipv4_hdr_t *h, const void *payload, size_t len) {
    (void)t; (void)h; (void)payload; (void)len;
}
static inline int ipv4_recv(tty_t *t, ipv4_hdr_t *h, void *payload, size_t len) {
    (void)t; (void)h; (void)payload; (void)len; return -ENOSYS;
}

#endif /* CONFIG_NET_IPV4_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_NET_IPV4_H */
