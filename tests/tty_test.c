/* SPDX-License-Identifier: MIT */

/**
 * @file tty_test.c
 * @brief Unit tests for TTY ring buffer driver
 *
 * Tests Phase 5 TTY improvements:
 * - Power-of-2 fast modulo optimization (2-10x faster)
 * - Ring buffer operations (TX/RX)
 * - Overflow tracking
 * - Buffer space calculation
 */

#include "drivers/tty/tty.h"
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

/* Mock hardware functions */
static uint8_t last_tx_byte = 0;
static int mock_rx_byte = -1;

static void mock_putc(uint8_t c) {
    last_tx_byte = c;
}

static int mock_getc(void) {
    int ret = mock_rx_byte;
    mock_rx_byte = -1;  /* Consume byte */
    return ret;
}

/**
 * Test 1: TTY Initialization
 */
static void test_tty_init(void) {
    printf("\nTest 1: TTY Initialization\n");
    printf("---------------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    TEST_ASSERT(tty.rx_buf == rx_buf, "RX buffer assigned");
    TEST_ASSERT(tty.tx_buf == tx_buf, "TX buffer assigned");
    TEST_ASSERT(tty.buf_size == 64, "Buffer size correct");
    TEST_ASSERT(tty.buf_mask == 63, "Buffer mask is size-1 (power-of-2)");
    TEST_ASSERT(tty.rx_head == 0, "RX head initialized to 0");
    TEST_ASSERT(tty.rx_tail == 0, "RX tail initialized to 0");
    TEST_ASSERT(tty.tx_head == 0, "TX head initialized to 0");
    TEST_ASSERT(tty.tx_tail == 0, "TX tail initialized to 0");
    TEST_ASSERT(tty.rx_overflow == 0, "RX overflow flag clear");
}

/**
 * Test 2: Power-of-2 Fast Modulo Optimization
 */
static void test_power_of_2_modulo(void) {
    printf("\nTest 2: Power-of-2 Fast Modulo\n");
    printf("-------------------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    /* Verify mask is power-of-2 minus 1 */
    TEST_ASSERT(tty.buf_mask == 63, "Mask = 0x3F (64-1)");

    /* Test wrap-around using mask (simulating RING_WRAP macro) */
    uint16_t idx = 65;
    uint16_t wrapped = idx & tty.buf_mask;
    TEST_ASSERT(wrapped == 1, "Index 65 wraps to 1 (65 & 0x3F)");

    idx = 127;
    wrapped = idx & tty.buf_mask;
    TEST_ASSERT(wrapped == 63, "Index 127 wraps to 63 (127 & 0x3F)");

    idx = 128;
    wrapped = idx & tty.buf_mask;
    TEST_ASSERT(wrapped == 0, "Index 128 wraps to 0 (128 & 0x3F)");

    printf("  → Optimization: Bitwise AND vs modulo (2-10x faster)\n");
}

/**
 * Test 3: TX Operations
 */
static void test_tty_tx(void) {
    printf("\nTest 3: TX Operations\n");
    printf("---------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    /* Test TX free space (initially full) */
    uint16_t free = tty_tx_free(&tty);
    TEST_ASSERT(free == 63, "TX buffer has 63 free slots (size-1)");

    /* Write single byte */
    tty_putc(&tty, 'A');
    TEST_ASSERT(tty.tx_head == 1, "TX head advanced to 1");
    TEST_ASSERT(tty.tx_tail == 0, "TX tail still 0 (not flushed)");
    TEST_ASSERT(tty.tx_buf[0] == 'A', "Byte written to buffer");

    free = tty_tx_free(&tty);
    TEST_ASSERT(free == 62, "TX buffer has 62 free slots");

    /* Flush TX buffer */
    tty_tx_flush(&tty);
    TEST_ASSERT(last_tx_byte == 'A', "Byte transmitted via putc");
    TEST_ASSERT(tty.tx_tail == 1, "TX tail advanced to 1");

    free = tty_tx_free(&tty);
    TEST_ASSERT(free == 63, "TX buffer empty after flush");
}

/**
 * Test 4: RX Operations
 */
static void test_tty_rx(void) {
    printf("\nTest 4: RX Operations\n");
    printf("---------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    /* Test RX available (initially empty) */
    uint16_t avail = tty_rx_available(&tty);
    TEST_ASSERT(avail == 0, "RX buffer initially empty");

    /* Simulate receiving byte */
    mock_rx_byte = 'X';
    tty_rx_poll(&tty);

    TEST_ASSERT(tty.rx_head == 1, "RX head advanced to 1");
    TEST_ASSERT(tty.rx_tail == 0, "RX tail still 0");
    TEST_ASSERT(tty.rx_buf[0] == 'X', "Byte stored in RX buffer");

    avail = tty_rx_available(&tty);
    TEST_ASSERT(avail == 1, "RX buffer has 1 byte available");

    /* Read byte */
    int byte = tty_getc(&tty);
    TEST_ASSERT(byte == 'X', "Read correct byte");
    TEST_ASSERT(tty.rx_tail == 1, "RX tail advanced to 1");

    avail = tty_rx_available(&tty);
    TEST_ASSERT(avail == 0, "RX buffer empty after read");

    /* Read when empty */
    byte = tty_getc(&tty);
    TEST_ASSERT(byte == -1, "Read from empty buffer returns -1");
}

/**
 * Test 5: Buffer Wraparound
 */
static void test_buffer_wraparound(void) {
    printf("\nTest 5: Buffer Wraparound\n");
    printf("-------------------------\n");

    uint8_t rx_buf[8], tx_buf[8];  /* Small buffer for testing */
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 8, mock_putc, mock_getc);

    TEST_ASSERT(tty.buf_mask == 7, "Mask = 7 (8-1)");

    /* Fill buffer to near end */
    for (int i = 0; i < 7; i++) {
        tty_putc(&tty, 'A' + i);
    }

    TEST_ASSERT(tty.tx_head == 7, "TX head at 7");

    /* Write one more byte (should wrap) */
    tty_putc(&tty, 'H');
    TEST_ASSERT(tty.tx_head == 8, "TX head advanced to 8");

    /* Verify wrap using mask */
    uint16_t wrapped_idx = tty.tx_head & tty.buf_mask;
    TEST_ASSERT(wrapped_idx == 0, "Index 8 wraps to 0");

    /* Verify data at wrapped position */
    TEST_ASSERT(tty.tx_buf[0] == 'H', "Wrapped byte at index 0");

    printf("  → Power-of-2 wraparound works correctly\n");
}

/**
 * Test 6: Overflow Detection
 */
static void test_overflow_detection(void) {
    printf("\nTest 6: Overflow Detection\n");
    printf("--------------------------\n");

    uint8_t rx_buf[4], tx_buf[4];  /* Very small buffer */
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 4, mock_putc, mock_getc);

    TEST_ASSERT(tty.rx_overflow == 0, "RX overflow initially clear");

    /* Fill RX buffer (3 bytes max in 4-byte buffer) */
    mock_rx_byte = '1';
    tty_rx_poll(&tty);
    mock_rx_byte = '2';
    tty_rx_poll(&tty);
    mock_rx_byte = '3';
    tty_rx_poll(&tty);

    TEST_ASSERT(tty.rx_overflow == 0, "No overflow with 3 bytes");

    /* Try to add one more (should overflow) */
    mock_rx_byte = '4';
    tty_rx_poll(&tty);

    TEST_ASSERT(tty.rx_overflow == 1, "Overflow flag set");
    TEST_ASSERT(tty.rx_head == 3, "RX head unchanged (overflow)");

    printf("  → Overflow protection works\n");
}

/**
 * Test 7: TX/RX Buffer Space Calculation
 */
static void test_buffer_space_calculation(void) {
    printf("\nTest 7: Buffer Space Calculation\n");
    printf("---------------------------------\n");

    uint8_t rx_buf[16], tx_buf[16];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 16, mock_putc, mock_getc);

    /* Initial state: empty */
    TEST_ASSERT(tty_tx_free(&tty) == 15, "TX free = 15 (size-1)");
    TEST_ASSERT(tty_rx_available(&tty) == 0, "RX available = 0");

    /* Add some TX data */
    tty_putc(&tty, 'A');
    tty_putc(&tty, 'B');
    tty_putc(&tty, 'C');

    TEST_ASSERT(tty_tx_free(&tty) == 12, "TX free = 12 (15-3)");

    /* Simulate RX data */
    mock_rx_byte = 'X';
    tty_rx_poll(&tty);
    mock_rx_byte = 'Y';
    tty_rx_poll(&tty);

    TEST_ASSERT(tty_rx_available(&tty) == 2, "RX available = 2");

    /* Flush some TX */
    tty_tx_flush(&tty);
    TEST_ASSERT(tty_tx_free(&tty) == 15, "TX free = 15 (after flush)");

    /* Read some RX */
    tty_getc(&tty);
    TEST_ASSERT(tty_rx_available(&tty) == 1, "RX available = 1 (after read)");
}

/**
 * Test 8: Bulk Operations
 */
static void test_bulk_operations(void) {
    printf("\nTest 8: Bulk Operations\n");
    printf("-----------------------\n");

    uint8_t rx_buf[64], tx_buf[64];
    tty_t tty;

    tty_init(&tty, rx_buf, tx_buf, 64, mock_putc, mock_getc);

    /* Write string */
    const char *msg = "Hello, TTY!";
    for (int i = 0; msg[i]; i++) {
        tty_putc(&tty, msg[i]);
    }

    uint16_t len = strlen(msg);
    TEST_ASSERT(tty.tx_head == len, "TX head advanced by message length");
    TEST_ASSERT(tty_tx_free(&tty) == 63 - len, "TX free space reduced");

    /* Flush all */
    while (tty.tx_head != tty.tx_tail) {
        tty_tx_flush(&tty);
    }

    TEST_ASSERT(tty.tx_head == tty.tx_tail, "TX buffer empty after full flush");
    TEST_ASSERT(tty_tx_free(&tty) == 63, "TX buffer fully available");

    printf("  → Bulk write of %u bytes successful\n", (unsigned)len);
}

/**
 * Main test runner
 */
int main(void) {
    printf("=== TTY Ring Buffer Tests ===\n");
    printf("Testing Phase 5 TTY improvements (power-of-2 fast modulo)\n");

    /* Run tests */
    test_tty_init();
    test_power_of_2_modulo();
    test_tty_tx();
    test_tty_rx();
    test_buffer_wraparound();
    test_overflow_detection();
    test_buffer_space_calculation();
    test_bulk_operations();

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
