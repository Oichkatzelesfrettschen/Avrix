/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file vfs.h
 * @brief Virtual Filesystem (VFS) Layer
 *
 * Unified interface for multiple filesystem types (ROMFS, EEPFS, etc.).
 * Provides mount-based path resolution and filesystem-agnostic operations.
 *
 * ## Novel Design Features
 *
 * **1. Lightweight Dispatch via Function Pointers**
 * - Each filesystem registers operations (open, read, write)
 * - VFS dispatches to correct FS at runtime
 * - Zero overhead when only one FS is used (compiler optimizes)
 *
 * **2. Mount Point System**
 * - Mount filesystems at virtual paths (/rom, /eep, /flash, etc.)
 * - Automatic path resolution and routing
 * - Hierarchical namespace unification
 *
 * **3. Unified File Descriptor**
 * - Single fd_t type for all filesystems
 * - Filesystem type embedded in descriptor
 * - Seamless cross-filesystem operations
 *
 * **4. Zero-Copy Where Possible**
 * - Read operations avoid intermediate buffers
 * - Direct dispatch to underlying filesystem
 * - Flash-to-RAM copy only when necessary
 *
 * ## Usage
 *
 * ```c
 * // Initialize VFS
 * vfs_init();
 *
 * // Mount filesystems
 * vfs_mount(VFS_TYPE_ROMFS, "/rom");
 * vfs_mount(VFS_TYPE_EEPFS, "/eep");
 *
 * // Open files from different filesystems
 * int fd1 = vfs_open("/rom/etc/version.txt", O_RDONLY);
 * int fd2 = vfs_open("/eep/sys/message.txt", O_RDWR);
 *
 * // Read/write operations (filesystem-agnostic)
 * uint8_t buf[32];
 * vfs_read(fd1, buf, sizeof(buf));
 * vfs_write(fd2, "Updated!", 8);
 *
 * // Close files
 * vfs_close(fd1);
 * vfs_close(fd2);
 * ```
 *
 * ## Memory Footprint
 * - Flash: ~300 bytes (dispatch layer)
 * - RAM: 8 bytes per mount point + file descriptor table
 * - Stack: ~20 bytes during operations
 */

#ifndef DRIVERS_FS_VFS_H
#define DRIVERS_FS_VFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Maximum number of mount points
 *
 * Increase if you need more filesystems mounted simultaneously.
 * Each mount point costs 8 bytes of RAM.
 */
#ifndef VFS_MAX_MOUNTS
#  define VFS_MAX_MOUNTS 4
#endif

/**
 * @brief Maximum number of open file descriptors
 *
 * Trade-off: More FDs = more RAM usage, more concurrent files.
 * Each FD costs ~12 bytes of RAM.
 */
#ifndef VFS_MAX_FDS
#  define VFS_MAX_FDS 8
#endif

/**
 * @brief Maximum path length (including null terminator)
 */
#ifndef VFS_MAX_PATH
#  define VFS_MAX_PATH 64
#endif

_Static_assert(VFS_MAX_MOUNTS >= 1, "Need at least 1 mount point");
_Static_assert(VFS_MAX_FDS >= 1, "Need at least 1 file descriptor");

/*═══════════════════════════════════════════════════════════════════
 * FILESYSTEM TYPES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Filesystem type identifiers
 */
typedef enum {
    VFS_TYPE_NONE = 0,   /**< Invalid/unmounted */
    VFS_TYPE_ROMFS,      /**< Read-only memory filesystem (flash) */
    VFS_TYPE_EEPFS,      /**< EEPROM filesystem (persistent) */
    VFS_TYPE_RAMFS,      /**< RAM filesystem (volatile, future) */
    VFS_TYPE_FATFS,      /**< FAT filesystem (SD card, future) */
} vfs_type_t;

/*═══════════════════════════════════════════════════════════════════
 * FILE OPERATIONS (POSIX-like)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief File open flags (POSIX subset)
 */
#define O_RDONLY   0x00  /**< Read-only */
#define O_WRONLY   0x01  /**< Write-only */
#define O_RDWR     0x02  /**< Read-write */
#define O_CREAT    0x40  /**< Create if doesn't exist (future) */
#define O_TRUNC    0x80  /**< Truncate to zero length (future) */

/**
 * @brief Seek origin (POSIX standard)
 */
#define SEEK_SET   0  /**< Seek from beginning */
#define SEEK_CUR   1  /**< Seek from current position */
#define SEEK_END   2  /**< Seek from end */

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - VFS MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize VFS subsystem
 *
 * Must be called once at system startup before any VFS operations.
 * Clears mount table and file descriptor table.
 */
void vfs_init(void);

/**
 * @brief Mount a filesystem at a mount point
 *
 * @param type Filesystem type (VFS_TYPE_ROMFS, VFS_TYPE_EEPFS, etc.)
 * @param path Mount point path (e.g., "/rom", "/eep")
 * @return 0 on success, -1 on error
 *
 * @note Mount point path must start with '/'
 * @note Cannot mount at "/" (reserved for root)
 * @note Cannot mount over existing mount point
 *
 * Example:
 * ```c
 * vfs_mount(VFS_TYPE_ROMFS, "/rom");   // Access via /rom/*
 * vfs_mount(VFS_TYPE_EEPFS, "/eep");   // Access via /eep/*
 * ```
 */
int vfs_mount(vfs_type_t type, const char *path);

/**
 * @brief Unmount a filesystem
 *
 * @param path Mount point path
 * @return 0 on success, -1 on error
 *
 * @note Fails if any files from this filesystem are open
 */
int vfs_unmount(const char *path);

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - FILE OPERATIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Open a file
 *
 * @param path Absolute path (e.g., "/rom/etc/version.txt")
 * @param flags Open flags (O_RDONLY, O_RDWR, etc.)
 * @return File descriptor (>= 0) on success, -1 on error
 *
 * @note Path must start with a mount point (e.g., /rom, /eep)
 * @note File descriptor must be closed with vfs_close()
 *
 * Path Resolution:
 * 1. Extract mount point from path ("/rom/file" -> mount "/rom")
 * 2. Find mounted filesystem for mount point
 * 3. Pass remaining path to filesystem ("file")
 * 4. Return unified file descriptor
 */
int vfs_open(const char *path, int flags);

/**
 * @brief Read from a file
 *
 * @param fd File descriptor returned by vfs_open()
 * @param buf Destination buffer
 * @param count Number of bytes to read
 * @return Number of bytes read, 0 on EOF, -1 on error
 *
 * @note Advances file position by number of bytes read
 * @note Reads less than `count` if EOF reached
 */
int vfs_read(int fd, void *buf, size_t count);

/**
 * @brief Write to a file
 *
 * @param fd File descriptor returned by vfs_open()
 * @param buf Source buffer
 * @param count Number of bytes to write
 * @return Number of bytes written, -1 on error
 *
 * @note Advances file position by number of bytes written
 * @note Fails if filesystem is read-only (e.g., ROMFS)
 * @note For EEPFS, uses wear-leveling update semantics
 */
int vfs_write(int fd, const void *buf, size_t count);

/**
 * @brief Seek to position in file
 *
 * @param fd File descriptor
 * @param offset Offset in bytes
 * @param whence Origin (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return New file position, -1 on error
 *
 * @note SEEK_SET: offset from beginning
 * @note SEEK_CUR: offset from current position
 * @note SEEK_END: offset from end (typically negative)
 */
int vfs_lseek(int fd, int offset, int whence);

/**
 * @brief Close a file descriptor
 *
 * @param fd File descriptor to close
 * @return 0 on success, -1 on error
 *
 * @note After closing, fd is invalid and can be reused
 */
int vfs_close(int fd);

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - FILE INFORMATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief File information structure
 */
typedef struct {
    uint16_t size;       /**< File size in bytes */
    uint8_t  type;       /**< File type (regular, directory, etc.) */
    uint8_t  flags;      /**< File flags (read-only, etc.) */
} vfs_stat_t;

/**
 * @brief Get file information
 *
 * @param path Absolute path
 * @param st Pointer to stat structure to fill
 * @return 0 on success, -1 on error
 */
int vfs_stat(const char *path, vfs_stat_t *st);

/**
 * @brief Get file information from open fd
 *
 * @param fd File descriptor
 * @param st Pointer to stat structure to fill
 * @return 0 on success, -1 on error
 */
int vfs_fstat(int fd, vfs_stat_t *st);

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - DEBUGGING & DIAGNOSTICS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief VFS statistics
 */
typedef struct {
    uint8_t mounts_used;     /**< Number of active mount points */
    uint8_t mounts_total;    /**< Maximum mount points */
    uint8_t fds_used;        /**< Number of open file descriptors */
    uint8_t fds_total;       /**< Maximum file descriptors */
} vfs_stats_t;

/**
 * @brief Get VFS statistics
 *
 * @param stats Pointer to stats structure to fill
 */
void vfs_get_stats(vfs_stats_t *stats);

/**
 * @brief Print mount table (for debugging)
 *
 * Outputs mount table to console/debug interface.
 * Useful for troubleshooting path resolution issues.
 */
void vfs_print_mounts(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FS_VFS_H */
