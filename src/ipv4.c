/*────────────────────────────── ipv4.c ─────────────────────────────
   Minimal IPv4 helpers for SLIP demos
   ---------------------------------------------------------------*/

#include "ipv4.h"
#include "slip.h"
#include <string.h>

uint16_t ipv4_checksum(const void *buf, size_t len)
{
    const uint8_t *p = buf;
    uint32_t sum = 0;
    while (len > 1) {
        sum += (uint16_t)(p[0] << 8 | p[1]);
        p += 2;
        len -= 2;
        if (sum > 0xFFFF)
            sum = (sum & 0xFFFF) + 1;
    }
    if (len) {
        sum += (uint16_t)(p[0] << 8);
        if (sum > 0xFFFF)
            sum = (sum & 0xFFFF) + 1;
    }
    return (uint16_t)~sum;
}

void ipv4_init_header(ipv4_hdr_t *h, uint32_t src, uint32_t dst,
                      uint8_t proto, uint16_t payload_len)
{
    memset(h, 0, sizeof *h);
    h->ver_ihl = 0x45;
    h->ttl     = 64;
    h->proto   = proto;
    h->len     = ip_htons(sizeof(ipv4_hdr_t) + payload_len);
    h->saddr   = ip_htonl(src);
    h->daddr   = ip_htonl(dst);
    h->checksum = 0;
    h->checksum = ipv4_checksum(h, sizeof *h);
}

void ipv4_send(tty_t *t, const ipv4_hdr_t *h, const void *payload, size_t len)
{
    uint8_t frame[sizeof *h + len];
    memcpy(frame, h, sizeof *h);
    memcpy(frame + sizeof *h, payload, len);
    slip_send_packet(t, frame, sizeof frame);
}

int ipv4_recv(tty_t *t, ipv4_hdr_t *h, void *payload, size_t len)
{
    uint8_t frame[256];
    int n = slip_recv_packet(t, frame, sizeof frame);
    if (n <= (int)sizeof(ipv4_hdr_t))
        return 0;
    memcpy(h, frame, sizeof *h);
    int plen = n - (int)sizeof(ipv4_hdr_t);
    if ((size_t)plen > len)
        plen = (int)len;
    memcpy(payload, frame + sizeof *h, plen);
    return plen;
}
