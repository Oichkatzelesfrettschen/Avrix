#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include <stddef.h>
#include "tty.h"

#ifdef __cplusplus
extern "C" {
#endif

/*────────────────────────────── ipv4.h ──────────────────────────────
 * Minimal IPv4 helpers for demo purposes.
 *   • 20-byte header only, no options
 *   • host-endian helpers for checksum and packing
 *────────────────────────────────────────────────────────────────────*/

typedef struct {
    uint8_t  ver_ihl;   /* 0x45 for IPv4 with 20-byte header */
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t saddr;
    uint32_t daddr;
} ipv4_hdr_t;

static inline uint16_t ip_htons(uint16_t x) { return (uint16_t)((x >> 8) | (x << 8)); }
static inline uint32_t ip_htonl(uint32_t x) {
    return ((x >> 24) & 0x000000FFu) |
           ((x >> 8)  & 0x0000FF00u) |
           ((x << 8)  & 0x00FF0000u) |
           ((x << 24) & 0xFF000000u);
}
#define ip_ntohs ip_htons
#define ip_ntohl ip_htonl

uint16_t ipv4_checksum(const void *buf, size_t len);
void ipv4_init_header(ipv4_hdr_t *h, uint32_t src, uint32_t dst,
                      uint8_t proto, uint16_t payload_len);
void ipv4_send(tty_t *t, const ipv4_hdr_t *h, const void *payload, size_t len);
int  ipv4_recv(tty_t *t, ipv4_hdr_t *h, void *payload, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* IPV4_H */
