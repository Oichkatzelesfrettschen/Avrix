#ifndef AVR_FS_H
#define AVR_FS_H

#include <stdint.h>

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
#define FS_BLOCK_SIZE 64

/** Number of blocks in the in-memory filesystem. */
#define FS_NUM_BLOCKS 128

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

void fs_init(void);
int  fs_create(const char *name, uint8_t type);
int  fs_open(const char *name, file_t *f);
int  fs_write(file_t *f, const void *buf, uint16_t len);
int  fs_read(file_t *f, void *buf, uint16_t len);
/**
 * List all valid filenames in the flat directory.
 *
 * \param[out] out  Array with ``FS_NUM_INODES`` elements for storing names.
 * \return          Number of entries copied into ``out``.
 */
int  fs_list(char (*out)[FS_MAX_NAME + 1]);

#ifdef __cplusplus
}
#endif

#endif /* AVR_FS_H */
