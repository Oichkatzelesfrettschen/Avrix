#ifndef AVR_FS_H
#define AVR_FS_H

#include <stdint.h>
#include <stddef.h>

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
/**
 * Populate *buf* with a newline separated list of valid filenames.
 *
 * The resulting string is always NUL terminated.  Names exceeding
 * ``len`` are truncated to fit.  The directory is flat so at most
 * ``FS_NUM_INODES`` entries are produced.
 *
 * \param[out] buf  Destination buffer for the concatenated list.
 * \param len       Buffer length in bytes.
 * \return          Number of filenames written.
 */
int  fs_list(char *buf, size_t len);

/** Release *name* and reclaim all associated blocks.
 *
 * Returns ``0`` on success or ``-1`` if the file does not exist.
 */
int  fs_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* AVR_FS_H */
