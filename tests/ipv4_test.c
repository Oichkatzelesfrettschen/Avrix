/* SPDX-License-Identifier: MIT */

/**
 * @file ipv4_test.c
 * @brief Unit tests for IPv4 protocol implementation
 */

#include "drivers/net/ipv4.h"
#include "drivers/net/slip.h"
#include "drivers/tty/tty.h"
#include "avrix-config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  ✓ %s\n", msg); \
        tests_passed++; \
    } else { \
        printf("  ✗ %s\n", msg); \
        tests_failed++; \
    } \
} while(0)

#if CONFIG_NET_IPV4_ENABLED

/**
 * Test 1: RFC 1071 Checksum Implementation
 */
static void test_ipv4_checksum(void) {
    printf("\nTest 1: RFC 1071 Checksum\n");
    printf("-------------------------\n");

    /* Test vector 1: Simple data */
    uint8_t data1[] = {0x00, 0x01, 0x02, 0x03};
    uint16_t sum1 = ipv4_checksum(data1, sizeof(data1));
    TEST_ASSERT(sum1 != 0, "Checksum computed for simple data");

    /* Test vector 2: All zeros should give 0xFFFF */
    uint8_t data2[] = {0x00, 0x00, 0x00, 0x00};
    uint16_t sum2 = ipv4_checksum(data2, sizeof(data2));
    TEST_ASSERT(sum2 == 0xFFFF, "Checksum of zeros is 0xFFFF");

    /* Test vector 3: Odd-length data */
    uint8_t data3[] = {0x12, 0x34, 0x56};
    uint16_t sum3 = ipv4_checksum(data3, sizeof(data3));
    TEST_ASSERT(sum3 != 0, "Checksum handles odd-length data");

    /* Test vector 4: Known IPv4 header */
    uint8_t header[] = {
        0x45, 0x00, 0x00, 0x3c,  /* ver/ihl, tos, len */
        0x1c, 0x46, 0x40, 0x00,  /* id, flags/frag */
        0x40, 0x06, 0x00, 0x00,  /* ttl, proto, checksum (0) */
        0xac, 0x10, 0x0a, 0x63,  /* src */
        0xac, 0x10, 0x0a, 0x0c   /* dst */
    };
    uint16_t sum4 = ipv4_checksum(header, 20);
    TEST_ASSERT(sum4 != 0, "IPv4 header checksum computed");

    printf("  → Checksum: 0x%04X\n", sum4);
}

/**
 * Test 2: Header Initialization
 */
static void test_ipv4_header_init(void) {
    printf("\nTest 2: Header Initialization\n");
    printf("------------------------------\n");

    ipv4_hdr_t hdr;
    /* Initialize to avoid uninitialized warnings in case stub does nothing */
    memset(&hdr, 0, sizeof(hdr));

    uint32_t src_ip = 0x0A000001;  /* 10.0.0.1 */
    uint32_t dst_ip = 0x0A000002;  /* 10.0.0.2 */
    uint16_t payload_len = 100;

    ipv4_init_header(&hdr, src_ip, dst_ip, IPV4_PROTO_UDP, payload_len);

    /* Verify fields */
    TEST_ASSERT(hdr.ver_ihl == 0x45, "Version/IHL is 0x45");
    TEST_ASSERT(hdr.proto == IPV4_PROTO_UDP, "Protocol is UDP (17)");
    TEST_ASSERT(hdr.ttl == 64, "TTL is 64");

    /* Verify length (header + payload) */
    uint16_t total_len = ipv4_ntohs(hdr.len);
    TEST_ASSERT(total_len == 20 + payload_len, "Total length correct");

    /* Verify checksum is non-zero */
    TEST_ASSERT(hdr.checksum != 0, "Checksum computed");

    /* Verify addresses (network byte order) */
    uint32_t src_net = ipv4_ntohl(hdr.saddr);
    uint32_t dst_net = ipv4_ntohl(hdr.daddr);
    TEST_ASSERT(src_net == src_ip, "Source IP correct");
    TEST_ASSERT(dst_net == dst_ip, "Destination IP correct");
}

/**
 * Test 3: Header Validation
 */
static void test_ipv4_header_validation(void) {
    printf("\nTest 3: Header Validation\n");
    printf("-------------------------\n");

    ipv4_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));

    /* Valid header */
    ipv4_init_header(&hdr, 0x0A000001, 0x0A000002, IPV4_PROTO_ICMP, 50);
    TEST_ASSERT(ipv4_validate_header(&hdr), "Valid header passes validation");

    /* Corrupt version */
    ipv4_hdr_t bad_ver = hdr;
    bad_ver.ver_ihl = 0x35;  /* Version 3 instead of 4 */
    TEST_ASSERT(!ipv4_validate_header(&bad_ver), "Invalid version rejected");

    /* Corrupt IHL */
    ipv4_hdr_t bad_ihl = hdr;
    bad_ihl.ver_ihl = 0x46;  /* IHL = 6 (options not supported) */
    TEST_ASSERT(!ipv4_validate_header(&bad_ihl), "Invalid IHL rejected");

    /* Corrupt checksum */
    ipv4_hdr_t bad_csum = hdr;
    bad_csum.checksum = ipv4_htons(0x1234);  /* Wrong checksum */
    TEST_ASSERT(!ipv4_validate_header(&bad_csum), "Invalid checksum rejected");

    /* Length too small */
    ipv4_hdr_t bad_len = hdr;
    bad_len.len = ipv4_htons(10);  /* Less than 20 bytes */
    TEST_ASSERT(!ipv4_validate_header(&bad_len), "Invalid length rejected");
}

/**
 * Test 4: Endianness Conversions
 */
static void test_ipv4_endianness(void) {
    printf("\nTest 4: Endianness Conversions\n");
    printf("-------------------------------\n");

    /* Test htons/ntohs */
    uint16_t h16 = 0x1234;
    uint16_t n16 = ipv4_htons(h16);
    uint16_t h16_back = ipv4_ntohs(n16);
    TEST_ASSERT(h16_back == h16, "htons/ntohs round-trip");

    /* Verify byte order swap on little-endian */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    TEST_ASSERT(n16 == 0x3412, "htons swaps bytes on little-endian");
#else
    TEST_ASSERT(n16 == 0x1234, "htons is identity on big-endian");
#endif

    /* Test htonl/ntohl */
    uint32_t h32 = 0x12345678;
    uint32_t n32 = ipv4_htonl(h32);
    uint32_t h32_back = ipv4_ntohl(n32);
    TEST_ASSERT(h32_back == h32, "htonl/ntohl round-trip");

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    TEST_ASSERT(n32 == 0x78563412, "htonl swaps bytes on little-endian");
#else
    TEST_ASSERT(n32 == 0x12345678, "htonl is identity on big-endian");
#endif
}

/**
 * Test 5: Protocol Numbers
 */
static void test_ipv4_protocols(void) {
    printf("\nTest 5: Protocol Numbers\n");
    printf("------------------------\n");

    TEST_ASSERT(IPV4_PROTO_ICMP == 1, "ICMP protocol number is 1");
    TEST_ASSERT(IPV4_PROTO_TCP == 6, "TCP protocol number is 6");
    TEST_ASSERT(IPV4_PROTO_UDP == 17, "UDP protocol number is 17");
}

/* Mock TTY (no actual transmission) */
static uint8_t tx_count = 0;
static void mock_putc(uint8_t c) { (void)c; tx_count++; }
static int mock_getc(void) { return -1; }

/**
 * Test 6: Transmission (with mock TTY)
 */
static void test_ipv4_transmission(void) {
    printf("\nTest 6: Packet Transmission\n");
    printf("----------------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;
    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    /* Send a packet */
    ipv4_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    ipv4_init_header(&hdr, 0x0A000001, 0x0A000002, IPV4_PROTO_UDP, 10);

    uint8_t payload[10] = "TEST DATA";
    tx_count = 0;

    ipv4_send(&tty, &hdr, payload, sizeof(payload));

    TEST_ASSERT(tx_count > 0, "Packet transmitted (bytes sent)");
    TEST_ASSERT(tx_count >= 20 + 10, "Sent at least header + payload");
    printf("  → Bytes transmitted: %u (includes SLIP framing)\n", tx_count);
}

#endif /* CONFIG_NET_IPV4_ENABLED */

/**
 * Main test runner
 */
int main(void) {
    printf("=== IPv4 Unit Tests ===\n");
#if CONFIG_NET_IPV4_ENABLED
    printf("Testing Phase 5 IPv4 improvements (RFC 1071 + validation)\n");

    /* Run tests */
    test_ipv4_checksum();
    test_ipv4_header_init();
    test_ipv4_header_validation();
    test_ipv4_endianness();
    test_ipv4_protocols();
    test_ipv4_transmission();
#else
    printf("Skipping tests: IPv4 disabled in config\n");
#endif

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
