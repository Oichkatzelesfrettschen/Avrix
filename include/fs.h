/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef AVR_FS_H
#define AVR_FS_H

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "avrix-config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file fs.h
 * @brief Minimal V7-style filesystem structures and API for the µ-UNIX
 *        platform running on an ATmega328P. The implementation stores the
 *        filesystem entirely in SRAM for illustrative purposes.
 */

/** Size of a filesystem block in bytes. Chosen small to conserve memory. */
#define FS_BLOCK_SIZE 32

/** Number of blocks in the in-memory filesystem. */
#define FS_NUM_BLOCKS 16

/** Maximum number of inodes supported. */
#define FS_NUM_INODES 16

/** Maximum length of a filename (excluding null terminator). */
#define FS_MAX_NAME 14

/** On-disk inode structure mirroring UNIX V7. */
typedef struct {
    uint8_t  type;                /**< 0 = free, 1 = file, 2 = directory */
    uint8_t  nlink;               /**< Reference count */
    uint16_t size;                /**< File size in bytes */
    uint16_t addrs[4];            /**< Direct block addresses */
} dinode_t;

/** File handle used by the simple API. */
typedef struct {
    uint8_t  inum;                /**< inode number */
    uint16_t off;                 /**< byte offset for reads/writes */
} file_t;

#if CONFIG_FS_ENABLED

void fs_init(void);
int  fs_create(const char *name, uint8_t type);
int  fs_open(const char *name, file_t *f);
int  fs_write(file_t *f, const void *buf, uint16_t len);
int  fs_read(file_t *f, void *buf, uint16_t len);
int  fs_list(char *buf, size_t len);
int  fs_unlink(const char *name);

#else /* !CONFIG_FS_ENABLED - Stubs for POSIX compliance */

static inline void fs_init(void) {}

static inline int fs_create(const char *name, uint8_t type) {
    (void)name; (void)type;
    return -ENOSYS;
}

static inline int fs_open(const char *name, file_t *f) {
    (void)name; (void)f;
    return -ENOSYS;
}

static inline int fs_write(file_t *f, const void *buf, uint16_t len) {
    (void)f; (void)buf; (void)len;
    return -ENOSYS;
}

static inline int fs_read(file_t *f, void *buf, uint16_t len) {
    (void)f; (void)buf; (void)len;
    return -ENOSYS;
}

static inline int fs_list(char *buf, size_t len) {
    (void)buf; (void)len;
    return -ENOSYS;
}

static inline int fs_unlink(const char *name) {
    (void)name;
    return -ENOSYS;
}

#endif /* CONFIG_FS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* AVR_FS_H */
