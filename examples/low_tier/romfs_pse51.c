/* SPDX-License-Identifier: MIT */

/**
 * @file romfs_pse51.c
 * @brief PSE51 Minimal Profile - Read-Only Filesystem Demo
 *
 * Demonstrates PSE51 file I/O using ROMFS:
 * - Read-only file access
 * - Zero RAM overhead (metadata in flash)
 * - Suitable for configuration files, lookup tables
 *
 * Target: Low-end MCUs with external flash/ROM
 * Profile: PSE51 + minimal file I/O
 *
 * Memory Footprint:
 * - Flash: ~300 bytes (ROMFS + demo)
 * - RAM: ~32 bytes (file descriptor + buffer)
 * - EEPROM: 0 bytes
 */

#include "drivers/fs/romfs.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Simulated ROMFS data in flash
 *
 * In production, this would be:
 * - External SPI flash
 * - MCU program memory (PROGMEM on AVR)
 * - Memory-mapped ROM
 */
static const uint8_t config_data[] = {
    'v', 'e', 'r', 's', 'i', 'o', 'n', '=', '1', '.', '0', '\n',
    'm', 'o', 'd', 'e', '=', 'd', 'e', 'b', 'u', 'g', '\n',
    0
};

/**
 * @brief PSE51 ROMFS demonstration
 */
int main(void) {
    printf("=== PSE51 ROMFS Demo ===\n\n");

    /* Initialize ROMFS (would scan directory in real impl) */
    printf("Initializing ROMFS...\n");
    printf("  Location: Flash/ROM @ 0x%p\n", (void *)config_data);
    printf("  Size: %zu bytes\n", sizeof(config_data));

    /* Simulate file open (ROMFS uses const pointers, zero RAM) */
    romfs_file_t config_file = {
        .data = config_data,
        .size = sizeof(config_data) - 1  /* Exclude null terminator */
    };

    printf("\nReading configuration file:\n");
    printf("  Handle: 0x%p\n", (void *)&config_file);
    printf("  Size: %u bytes\n", config_file.size);
    printf("  RAM overhead: 0 bytes (const data)\n\n");

    /* Read file contents */
    printf("Contents:\n");
    printf("---\n");

    uint8_t buffer[64];
    int bytes_read = romfs_read(&config_file, 0, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';  /* Null terminate for printf */
        printf("%s", buffer);
    }

    printf("---\n\n");

    /* Demonstrate seeking/partial reads */
    printf("Partial read (offset 8, 10 bytes):\n");
    bytes_read = romfs_read(&config_file, 8, buffer, 10);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("  Read: \"%s\"\n", buffer);
    }

    /* PSE51: File operations summary */
    printf("\nPSE51 File I/O Characteristics:\n");
    printf("  ✓ Read-only access (ROMFS)\n");
    printf("  ✓ Zero RAM for metadata\n");
    printf("  ✓ Deterministic performance\n");
    printf("  ✓ No dynamic allocation\n");
    printf("  ✗ Write operations (use EEPFS for writes)\n");
    printf("  ✗ Directories (flat namespace only)\n");

    printf("\nPSE51 ROMFS demo complete.\n");
    return 0;
}

/**
 * PSE51 File I/O Subset:
 * ══════════════════════
 *
 * **Supported:**
 * - open() equivalent: romfs_open(path)
 * - read() equivalent: romfs_read(file, offset, buf, len)
 * - lseek() equivalent: Pass offset to romfs_read()
 * - close() equivalent: Implicit (no cleanup needed)
 *
 * **Not Supported (PSE51 limitations):**
 * - write() - Use EEPFS for persistent writes
 * - Directory operations (opendir, readdir)
 * - File metadata (stat, chmod, chown)
 * - Buffered I/O (fopen, fread, fwrite)
 *
 * **Alternative for Writes:**
 * See eeprom_pse51.c for EEPFS write operations
 */
