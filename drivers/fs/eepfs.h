/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file eepfs.h
 * @brief Portable EEPROM Filesystem (EEPFS)
 *
 * Read-only filesystem stored in EEPROM/non-volatile storage.
 * Similar structure to ROMFS but with EEPROM-specific optimizations.
 *
 * ## Design
 * - Directory entries: 4 bytes each (like ROMFS)
 * - Metadata stored in program memory (flash/ROM)
 * - File data stored in EEPROM
 * - Zero SRAM overhead (except during reads)
 *
 * ## EEPROM Considerations
 * - Limited write cycles (~100k on AVR)
 * - Use hal_eeprom_update_*() to extend lifetime
 * - Reads are fast (~3.4ms/byte on AVR)
 * - Consider wear-leveling for frequently written data
 *
 * ## Memory Footprint
 * - Flash: ~250 bytes code + directory metadata
 * - EEPROM: file data only
 * - RAM: 0 bytes (except during file operations)
 *
 * ## Usage
 * ```c
 * // Initialize EEPROM filesystem (one-time setup)
 * eepfs_format();  // Writes initial filesystem structure
 *
 * // Open and read file
 * const eepfs_file_t *f = eepfs_open("/sys/message.txt");
 * if (f) {
 *     uint8_t buf[32];
 *     int n = eepfs_read(f, 0, buf, sizeof(buf));
 *     // Use data in buf...
 * }
 * ```
 *
 * ## Filesystem Structure
 * ```
 * /
 * └── sys/
 *     └── message.txt
 * ```
 */

#ifndef DRIVERS_FS_EEPFS_H
#define DRIVERS_FS_EEPFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief File descriptor for EEPFS objects
 *
 * Describes a file in EEPFS. Unlike ROMFS, data is in EEPROM not flash.
 */
typedef struct {
    uint16_t addr;  /**< EEPROM address of file data */
    uint16_t size;  /**< File size in bytes */
} eepfs_file_t;

/**
 * @brief Open a file by absolute path
 *
 * Searches the EEPFS directory hierarchy for the specified file.
 *
 * @param path Absolute UNIX-style path (e.g., "/sys/message.txt")
 * @return Pointer to file descriptor, or NULL if not found
 *
 * @note The returned pointer points to static data in flash/ROM.
 * @note Path lookup is case-sensitive.
 * @note Maximum path segment length: 11 characters (+ null terminator).
 */
const eepfs_file_t *eepfs_open(const char *path);

/**
 * @brief Read from an EEPFS file
 *
 * Copies data from EEPROM into a RAM buffer. Similar to pread(2).
 *
 * @param f Pointer to file descriptor returned by eepfs_open()
 * @param off Offset into file (bytes)
 * @param buf Destination buffer in RAM
 * @param len Number of bytes to read
 * @return Number of bytes actually read (may be less than `len` if EOF)
 *
 * @note If `off` >= file size, returns 0.
 * @note If `off + len` > file size, reads up to EOF.
 * @note Uses HAL EEPROM functions for portable access.
 */
int eepfs_read(const eepfs_file_t *f, uint16_t off, void *buf, uint16_t len);

/**
 * @brief Write to an EEPFS file (if supported)
 *
 * Writes data from RAM buffer to EEPROM. Uses update semantics to
 * minimize EEPROM wear.
 *
 * @param f Pointer to file descriptor
 * @param off Offset into file (bytes)
 * @param buf Source buffer in RAM
 * @param len Number of bytes to write
 * @return Number of bytes actually written
 *
 * @note Uses hal_eeprom_update_block() to avoid unnecessary writes.
 * @note Cannot extend file size (writes truncated at EOF).
 * @note This is a write-through operation (no caching).
 *
 * @warning EEPROM has limited write cycles! Use sparingly.
 */
int eepfs_write(const eepfs_file_t *f, uint16_t off, const void *buf, uint16_t len);

/**
 * @brief Format EEPROM with initial filesystem structure
 *
 * Writes the initial file data to EEPROM. Should be called once
 * during first boot or factory reset.
 *
 * @note This writes to EEPROM and takes several seconds.
 * @note On ATmega328P: ~1 second for small filesystem.
 *
 * @warning Destructive operation! Overwrites existing EEPROM data.
 */
void eepfs_format(void);

/**
 * @brief Get EEPROM usage statistics
 *
 * @param[out] used_bytes Number of bytes used by filesystem
 * @param[out] total_bytes Total EEPROM size
 */
void eepfs_stats(uint16_t *used_bytes, uint16_t *total_bytes);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FS_EEPFS_H */
