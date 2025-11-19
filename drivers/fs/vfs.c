/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file vfs.c
 * @brief Virtual Filesystem Implementation
 *
 * Lightweight VFS layer with function pointer dispatch and mount-based
 * path resolution. Optimized for embedded systems.
 */

#include "vfs.h"
#include "romfs.h"
#include "eepfs.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * FILESYSTEM OPERATIONS INTERFACE
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Filesystem operations vtable
 *
 * Each filesystem type implements these operations.
 * VFS dispatches to the appropriate function based on mount type.
 */
typedef struct {
    const void *(*open)(const char *path);                    /* Open file */
    int (*read)(const void *f, uint16_t off, void *buf, uint16_t len);  /* Read */
    int (*write)(const void *f, uint16_t off, const void *buf, uint16_t len); /* Write (optional) */
    uint16_t (*size)(const void *f);                          /* Get file size */
} vfs_ops_t;

/*═══════════════════════════════════════════════════════════════════
 * ROMFS OPERATIONS WRAPPER
 *═══════════════════════════════════════════════════════════════════*/

static const void *romfs_vfs_open(const char *path) {
    return romfs_open(path);
}

static int romfs_vfs_read(const void *f, uint16_t off, void *buf, uint16_t len) {
    return romfs_read((const romfs_file_t *)f, off, buf, len);
}

static int romfs_vfs_write(const void *f, uint16_t off, const void *buf, uint16_t len) {
    (void)f; (void)off; (void)buf; (void)len;
    return -1;  /* ROMFS is read-only */
}

static uint16_t romfs_vfs_size(const void *f) {
    const romfs_file_t *file = (const romfs_file_t *)f;
    romfs_file_t tmp;
    memcpy(&tmp, file, sizeof(tmp));  /* Copy from flash */
    return tmp.size;
}

static const vfs_ops_t romfs_ops = {
    .open = romfs_vfs_open,
    .read = romfs_vfs_read,
    .write = romfs_vfs_write,
    .size = romfs_vfs_size
};

/*═══════════════════════════════════════════════════════════════════
 * EEPFS OPERATIONS WRAPPER
 *═══════════════════════════════════════════════════════════════════*/

static const void *eepfs_vfs_open(const char *path) {
    return eepfs_open(path);
}

static int eepfs_vfs_read(const void *f, uint16_t off, void *buf, uint16_t len) {
    return eepfs_read((const eepfs_file_t *)f, off, buf, len);
}

static int eepfs_vfs_write(const void *f, uint16_t off, const void *buf, uint16_t len) {
    return eepfs_write((const eepfs_file_t *)f, off, buf, len);
}

static uint16_t eepfs_vfs_size(const void *f) {
    const eepfs_file_t *file = (const eepfs_file_t *)f;
    eepfs_file_t tmp;
    memcpy(&tmp, file, sizeof(tmp));  /* Copy from flash */
    return tmp.size;
}

static const vfs_ops_t eepfs_ops = {
    .open = eepfs_vfs_open,
    .read = eepfs_vfs_read,
    .write = eepfs_vfs_write,
    .size = eepfs_vfs_size
};

/*═══════════════════════════════════════════════════════════════════
 * VFS INTERNAL STATE
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Mount point entry
 */
typedef struct {
    char path[16];           /**< Mount point path (e.g., "/rom") */
    vfs_type_t type;         /**< Filesystem type */
    const vfs_ops_t *ops;    /**< Operations vtable */
} vfs_mount_t;

/**
 * @brief File descriptor entry
 */
typedef struct {
    const void *fs_file;     /**< Filesystem-specific file handle */
    const vfs_ops_t *ops;    /**< Operations vtable for this file */
    uint16_t position;       /**< Current file position */
    uint8_t flags;           /**< Open flags (O_RDONLY, etc.) */
    bool in_use;             /**< True if FD is allocated */
} vfs_fd_t;

/**
 * @brief VFS global state
 */
static struct {
    vfs_mount_t mounts[VFS_MAX_MOUNTS];
    vfs_fd_t fds[VFS_MAX_FDS];
    bool initialized;
} vfs_state;

/*═══════════════════════════════════════════════════════════════════
 * HELPER: GET OPERATIONS FOR FILESYSTEM TYPE
 *═══════════════════════════════════════════════════════════════════*/

static const vfs_ops_t *get_ops(vfs_type_t type) {
    switch (type) {
        case VFS_TYPE_ROMFS: return &romfs_ops;
        case VFS_TYPE_EEPFS: return &eepfs_ops;
        default: return NULL;
    }
}

/*═══════════════════════════════════════════════════════════════════
 * HELPER: FIND MOUNT POINT FOR PATH
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Find mount point matching path prefix
 *
 * @param path Absolute path (e.g., "/rom/etc/file")
 * @param[out] fs_path Pointer to remaining path (e.g., "/etc/file")
 * @return Pointer to mount entry, or NULL if no match
 */
static vfs_mount_t *find_mount(const char *path, const char **fs_path) {
    if (!path || path[0] != '/') {
        return NULL;  /* Must be absolute path */
    }

    /* Try each mount point */
    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        vfs_mount_t *m = &vfs_state.mounts[i];
        if (m->type == VFS_TYPE_NONE) {
            continue;  /* Empty slot */
        }

        size_t mlen = strlen(m->path);
        if (strncmp(path, m->path, mlen) == 0) {
            /* Check if this is an exact mount point match */
            if (path[mlen] == '/' || path[mlen] == '\0') {
                *fs_path = path + mlen;  /* Skip mount point prefix */
                return m;
            }
        }
    }

    return NULL;  /* No matching mount */
}

/*═══════════════════════════════════════════════════════════════════
 * HELPER: ALLOCATE FILE DESCRIPTOR
 *═══════════════════════════════════════════════════════════════════*/

static int alloc_fd(void) {
    for (int i = 0; i < VFS_MAX_FDS; i++) {
        if (!vfs_state.fds[i].in_use) {
            return i;
        }
    }
    return -1;  /* No free descriptors */
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - VFS MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

void vfs_init(void) {
    /* Clear all state */
    memset(&vfs_state, 0, sizeof(vfs_state));
    vfs_state.initialized = true;
}

int vfs_mount(vfs_type_t type, const char *path) {
    if (!vfs_state.initialized) {
        return -1;
    }
    if (!path || path[0] != '/') {
        return -1;  /* Invalid mount point */
    }

    /* Get operations for this filesystem type */
    const vfs_ops_t *ops = get_ops(type);
    if (!ops) {
        return -1;  /* Unknown filesystem type */
    }

    /* Check if already mounted */
    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE &&
            strcmp(vfs_state.mounts[i].path, path) == 0) {
            return -1;  /* Already mounted at this path */
        }
    }

    /* Find free mount slot */
    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type == VFS_TYPE_NONE) {
            /* Allocate mount point */
            strncpy(vfs_state.mounts[i].path, path, sizeof(vfs_state.mounts[i].path) - 1);
            vfs_state.mounts[i].path[sizeof(vfs_state.mounts[i].path) - 1] = '\0';
            vfs_state.mounts[i].type = type;
            vfs_state.mounts[i].ops = ops;
            return 0;
        }
    }

    return -1;  /* No free mount slots */
}

int vfs_unmount(const char *path) {
    if (!vfs_state.initialized || !path) {
        return -1;
    }

    /* Find mount point */
    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE &&
            strcmp(vfs_state.mounts[i].path, path) == 0) {

            /* Check if any files are open from this filesystem */
            for (int fd = 0; fd < VFS_MAX_FDS; fd++) {
                if (vfs_state.fds[fd].in_use &&
                    vfs_state.fds[fd].ops == vfs_state.mounts[i].ops) {
                    return -1;  /* Files still open */
                }
            }

            /* Unmount */
            vfs_state.mounts[i].type = VFS_TYPE_NONE;
            vfs_state.mounts[i].ops = NULL;
            vfs_state.mounts[i].path[0] = '\0';
            return 0;
        }
    }

    return -1;  /* Mount point not found */
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - FILE OPERATIONS
 *═══════════════════════════════════════════════════════════════════*/

int vfs_open(const char *path, int flags) {
    if (!vfs_state.initialized) {
        return -1;
    }

    /* Resolve path to mount point */
    const char *fs_path;
    vfs_mount_t *mount = find_mount(path, &fs_path);
    if (!mount) {
        return -1;  /* No matching mount point */
    }

    /* Open file in underlying filesystem */
    const void *fs_file = mount->ops->open(fs_path);
    if (!fs_file) {
        return -1;  /* File not found */
    }

    /* Allocate file descriptor */
    int fd = alloc_fd();
    if (fd < 0) {
        return -1;  /* No free descriptors */
    }

    /* Initialize descriptor */
    vfs_state.fds[fd].fs_file = fs_file;
    vfs_state.fds[fd].ops = mount->ops;
    vfs_state.fds[fd].position = 0;
    vfs_state.fds[fd].flags = (uint8_t)flags;
    vfs_state.fds[fd].in_use = true;

    return fd;
}

int vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) {
        return -1;  /* Invalid file descriptor */
    }

    vfs_fd_t *f = &vfs_state.fds[fd];

    /* Dispatch to filesystem */
    int nread = f->ops->read(f->fs_file, f->position, buf, (uint16_t)count);
    if (nread > 0) {
        f->position += (uint16_t)nread;
    }

    return nread;
}

int vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) {
        return -1;  /* Invalid file descriptor */
    }

    vfs_fd_t *f = &vfs_state.fds[fd];

    /* Check if write is allowed */
    if ((f->flags & O_WRONLY) == 0 && (f->flags & O_RDWR) == 0) {
        return -1;  /* File not open for writing */
    }

    /* Dispatch to filesystem */
    int nwritten = f->ops->write(f->fs_file, f->position, buf, (uint16_t)count);
    if (nwritten > 0) {
        f->position += (uint16_t)nwritten;
    }

    return nwritten;
}

int vfs_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) {
        return -1;
    }

    vfs_fd_t *f = &vfs_state.fds[fd];
    uint16_t size = f->ops->size(f->fs_file);
    int new_pos = 0;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = f->position + offset;
            break;
        case SEEK_END:
            new_pos = size + offset;
            break;
        default:
            return -1;  /* Invalid whence */
    }

    /* Clamp to valid range [0, size] */
    if (new_pos < 0) {
        new_pos = 0;
    }
    if (new_pos > size) {
        new_pos = size;
    }

    f->position = (uint16_t)new_pos;
    return new_pos;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) {
        return -1;
    }

    /* Mark descriptor as free */
    vfs_state.fds[fd].in_use = false;
    vfs_state.fds[fd].fs_file = NULL;
    vfs_state.fds[fd].ops = NULL;

    return 0;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - FILE INFORMATION
 *═══════════════════════════════════════════════════════════════════*/

int vfs_stat(const char *path, vfs_stat_t *st) {
    if (!path || !st) {
        return -1;
    }

    /* Open file to get info */
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    int result = vfs_fstat(fd, st);
    vfs_close(fd);
    return result;
}

int vfs_fstat(int fd, vfs_stat_t *st) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use || !st) {
        return -1;
    }

    vfs_fd_t *f = &vfs_state.fds[fd];
    st->size = f->ops->size(f->fs_file);
    st->type = 0;  /* Regular file */
    st->flags = 0; /* No special flags */

    return 0;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - DEBUGGING & DIAGNOSTICS
 *═══════════════════════════════════════════════════════════════════*/

void vfs_get_stats(vfs_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->mounts_total = VFS_MAX_MOUNTS;
    stats->fds_total = VFS_MAX_FDS;
    stats->mounts_used = 0;
    stats->fds_used = 0;

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE) {
            stats->mounts_used++;
        }
    }

    for (int i = 0; i < VFS_MAX_FDS; i++) {
        if (vfs_state.fds[i].in_use) {
            stats->fds_used++;
        }
    }
}

void vfs_print_mounts(void) {
    /* This would typically print to debug console */
    /* For embedded systems without printf, this is a stub */
    /* On systems with console, implement with printf/puts */
}
