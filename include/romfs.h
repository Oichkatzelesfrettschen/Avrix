/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef ROMFS_H
#define ROMFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \file romfs.h
 *  \brief Tiny 4-byte per entry read-only filesystem stored entirely in
 *         program flash (ROM).
 *
 *  Directory entries occupy four bytes and reference either a subdirectory
 *  table or file record. All tables reside in flash to keep the RAM footprint
 *  close to zero. Path lookups traverse the tables directly from flash and
 *  return a handle describing the file.
 */

/** File descriptor for ROMFS objects. */
typedef struct {
    const uint8_t *data; /**< pointer into flash */
    uint16_t       size; /**< size in bytes     */
} romfs_file_t;

/** Open a file by absolute path ("/etc/config/version.txt").
 *  \param path Absolute UNIX style path.
 *  \return     Pointer to file descriptor or NULL when not found.
 */
const romfs_file_t *romfs_open(const char *path);

/** Read from a ROMFS file. Similar to pread(2). */
int romfs_read(const romfs_file_t *f, uint16_t off, void *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* ROMFS_H */
