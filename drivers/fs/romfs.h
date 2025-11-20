/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file romfs.h
 * @brief Portable Read-Only Memory Filesystem (ROMFS)
 *
 * Tiny read-only filesystem stored entirely in program flash/ROM.
 * Originally designed for AVR (4 bytes per entry), now portable via HAL.
 *
 * ## Design
 * - Directory entries: 4 bytes each
 * - All metadata stored in flash/ROM (zero RAM overhead)
 * - Path lookups traverse tables directly from flash
 * - Simple hierarchical directory structure
 *
 * ## Memory Footprint
 * - Flash: ~200 bytes code + filesystem data
 * - RAM: 0 bytes (metadata in flash)
 * - Stack: ~20 bytes during lookup
 *
 * ## Usage
 * ```c
 * // Open a file
 * const romfs_file_t *f = romfs_open("/etc/config/version.txt");
 * if (f) {
 *     uint8_t buf[32];
 *     int n = romfs_read(f, 0, buf, sizeof(buf));
 *     // Use data in buf...
 * }
 * ```
 *
 * ## Filesystem Structure
 * ```
 * /
 * ├── etc/
 * │   └── config/
 * │       └── version.txt
 * └── README
 * ```
 */

#ifndef DRIVERS_FS_ROMFS_H
#define DRIVERS_FS_ROMFS_H

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
 * @brief File descriptor for ROMFS objects
 *
 * Describes a file in the ROMFS. All data resides in flash/ROM.
 */
typedef struct {
    const uint8_t *data;  /**< Pointer to file data in flash/ROM */
    uint16_t size;        /**< File size in bytes */
} romfs_file_t;

/**
 * @brief Open a file by absolute path
 *
 * Searches the ROMFS directory hierarchy for the specified file.
 *
 * @param path Absolute UNIX-style path (e.g., "/etc/config/version.txt")
 * @return Pointer to file descriptor, or NULL if not found
 *
 * @note The returned pointer points to static data in flash/ROM.
 * @note Path lookup is case-sensitive.
 * @note Maximum path segment length: 11 characters (+ null terminator).
 */
const romfs_file_t *romfs_open(const char *path);

/**
 * @brief Read from a ROMFS file
 *
 * Copies data from flash/ROM into a RAM buffer. Similar to pread(2).
 *
 * @param f Pointer to file descriptor returned by romfs_open()
 * @param off Offset into file (bytes)
 * @param buf Destination buffer in RAM
 * @param len Number of bytes to read
 * @return Number of bytes actually read (may be less than `len` if EOF)
 *
 * @note If `off` >= file size, returns 0.
 * @note If `off + len` > file size, reads up to EOF.
 * @note On AVR, uses memcpy_P() for flash access.
 */
int romfs_read(const romfs_file_t *f, uint16_t off, void *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FS_ROMFS_H */
