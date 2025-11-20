/* SPDX-License-Identifier: MIT */

/**
 * @file hello_pse51.c
 * @brief PSE51 Minimal Profile - Hello World
 *
 * Demonstrates the absolute minimum PSE51 functionality:
 * - Single-threaded execution
 * - Basic I/O (printf equivalent)
 * - No threading, no IPC, no networking
 *
 * Target: Low-end MCUs (ATmega128, 8-16KB flash, 1-2KB RAM)
 * Profile: PSE51 (IEEE Std 1003.13-2003)
 *
 * Memory Footprint:
 * - Flash: ~200 bytes (minimal startup + string output)
 * - RAM: ~16 bytes (stack frame)
 * - EEPROM: 0 bytes
 */

#include <stdio.h>
#include <stdint.h>

/**
 * @brief PSE51 minimal entry point
 *
 * PSE51 Key Characteristics:
 * - Single-threaded (no pthread, no signals)
 * - Minimal file I/O (optional)
 * - No memory protection
 * - No process model
 * - Deterministic (suitable for hard real-time)
 */
int main(void) {
    /* PSE51: Basic formatted output */
    printf("Hello from PSE51 (Minimal Embedded POSIX)!\n");
    printf("Profile: Single-threaded, no IPC, no networking\n");
    printf("Target: Low-end MCUs (ATmega128, 8-16KB flash)\n");

    /* PSE51: Simple computation */
    uint32_t cycles = 0;
    for (uint8_t i = 0; i < 10; i++) {
        cycles += i;
    }
    printf("Computed: %lu cycles\n", cycles);

    printf("PSE51 demo complete.\n");
    return 0;
}

/**
 * PSE51 Profile Summary (IEEE 1003.13-2003):
 * ═══════════════════════════════════════════
 *
 * **Included:**
 * - Basic I/O: printf, puts, putchar
 * - Math library: Basic arithmetic
 * - String functions: strlen, strcmp, etc.
 * - Time: Basic monotonic clock (optional)
 *
 * **Excluded:**
 * - Threading (pthread)
 * - Signals (kill, sigaction)
 * - Process management (fork, exec)
 * - IPC (pipes, sockets, shared memory)
 * - File systems (beyond simple RAM-based)
 * - Dynamic memory (malloc optional, discouraged)
 *
 * **Use Cases:**
 * - Sensor nodes
 * - Simple data loggers
 * - LED controllers
 * - Basic measurement devices
 * - Single-function appliances
 */
