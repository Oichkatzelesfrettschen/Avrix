/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file vfs.c
 * @brief Virtual Filesystem Implementation
 */

#include "vfs.h"
#include "avrix-config.h"

#if CONFIG_FS_ENABLED

#include "romfs.h"
#include "eepfs.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * FILESYSTEM OPERATIONS INTERFACE
 *═══════════════════════════════════════════════════════════════════*/

typedef struct {
    const void *(*open)(const char *path);
    int (*read)(const void *f, uint16_t off, void *buf, uint16_t len);
    int (*write)(const void *f, uint16_t off, const void *buf, uint16_t len);
    uint16_t (*size)(const void *f);
} vfs_ops_t;

/*═══════════════════════════════════════════════════════════════════
 * ROMFS OPERATIONS WRAPPER
 *═══════════════════════════════════════════════════════════════════*/

#if CONFIG_FS_ROMFS_ENABLED
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
#endif /* CONFIG_FS_ROMFS_ENABLED */

/*═══════════════════════════════════════════════════════════════════
 * EEPFS OPERATIONS WRAPPER
 *═══════════════════════════════════════════════════════════════════*/

#if CONFIG_FS_EEPFS_ENABLED
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
#endif /* CONFIG_FS_EEPFS_ENABLED */

/*═══════════════════════════════════════════════════════════════════
 * VFS INTERNAL STATE
 *═══════════════════════════════════════════════════════════════════*/

typedef struct {
    char path[16];
    vfs_type_t type;
    const vfs_ops_t *ops;
} vfs_mount_t;

typedef struct {
    const void *fs_file;
    const vfs_ops_t *ops;
    uint16_t position;
    uint8_t flags;
    bool in_use;
} vfs_fd_t;

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
#if CONFIG_FS_ROMFS_ENABLED
        case VFS_TYPE_ROMFS: return &romfs_ops;
#endif
#if CONFIG_FS_EEPFS_ENABLED
        case VFS_TYPE_EEPFS: return &eepfs_ops;
#endif
        default: return NULL;
    }
}

/*═══════════════════════════════════════════════════════════════════
 * HELPER: FIND MOUNT POINT FOR PATH
 *═══════════════════════════════════════════════════════════════════*/

static vfs_mount_t *find_mount(const char *path, const char **fs_path) {
    if (!path || path[0] != '/') return NULL;

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        vfs_mount_t *m = &vfs_state.mounts[i];
        if (m->type == VFS_TYPE_NONE) continue;

        size_t mlen = strlen(m->path);
        if (strncmp(path, m->path, mlen) == 0) {
            if (path[mlen] == '/' || path[mlen] == '\0') {
                *fs_path = path + mlen;
                return m;
            }
        }
    }
    return NULL;
}

static int alloc_fd(void) {
    for (int i = 0; i < VFS_MAX_FDS; i++) {
        if (!vfs_state.fds[i].in_use) return i;
    }
    return -1;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API
 *═══════════════════════════════════════════════════════════════════*/

void vfs_init(void) {
    memset(&vfs_state, 0, sizeof(vfs_state));
    vfs_state.initialized = true;
}

int vfs_mount(vfs_type_t type, const char *path) {
    if (!vfs_state.initialized || !path || path[0] != '/') return -1;

    const vfs_ops_t *ops = get_ops(type);
    if (!ops) return -1;

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE &&
            strcmp(vfs_state.mounts[i].path, path) == 0) {
            return -1;
        }
    }

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type == VFS_TYPE_NONE) {
            strncpy(vfs_state.mounts[i].path, path, sizeof(vfs_state.mounts[i].path) - 1);
            vfs_state.mounts[i].path[sizeof(vfs_state.mounts[i].path) - 1] = '\0';
            vfs_state.mounts[i].type = type;
            vfs_state.mounts[i].ops = ops;
            return 0;
        }
    }
    return -1;
}

int vfs_unmount(const char *path) {
    if (!vfs_state.initialized || !path) return -1;

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE &&
            strcmp(vfs_state.mounts[i].path, path) == 0) {

            for (int fd = 0; fd < VFS_MAX_FDS; fd++) {
                if (vfs_state.fds[fd].in_use &&
                    vfs_state.fds[fd].ops == vfs_state.mounts[i].ops) {
                    return -1;
                }
            }

            vfs_state.mounts[i].type = VFS_TYPE_NONE;
            vfs_state.mounts[i].ops = NULL;
            vfs_state.mounts[i].path[0] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_open(const char *path, int flags) {
    if (!vfs_state.initialized) return -1;

    const char *fs_path;
    vfs_mount_t *mount = find_mount(path, &fs_path);
    if (!mount) return -1;

    const void *fs_file = mount->ops->open(fs_path);
    if (!fs_file) return -1;

    int fd = alloc_fd();
    if (fd < 0) return -1;

    vfs_state.fds[fd].fs_file = fs_file;
    vfs_state.fds[fd].ops = mount->ops;
    vfs_state.fds[fd].position = 0;
    vfs_state.fds[fd].flags = (uint8_t)flags;
    vfs_state.fds[fd].in_use = true;

    return fd;
}

int vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) return -1;

    vfs_fd_t *f = &vfs_state.fds[fd];
    int nread = f->ops->read(f->fs_file, f->position, buf, (uint16_t)count);
    if (nread > 0) f->position += (uint16_t)nread;
    return nread;
}

int vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) return -1;

    vfs_fd_t *f = &vfs_state.fds[fd];
    if ((f->flags & O_WRONLY) == 0 && (f->flags & O_RDWR) == 0) return -1;

    int nwritten = f->ops->write(f->fs_file, f->position, buf, (uint16_t)count);
    if (nwritten > 0) f->position += (uint16_t)nwritten;
    return nwritten;
}

int vfs_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) return -1;

    vfs_fd_t *f = &vfs_state.fds[fd];
    uint16_t size = f->ops->size(f->fs_file);
    int new_pos = 0;

    switch (whence) {
        case SEEK_SET: new_pos = offset; break;
        case SEEK_CUR: new_pos = f->position + offset; break;
        case SEEK_END: new_pos = size + offset; break;
        default: return -1;
    }

    if (new_pos < 0) new_pos = 0;
    if (new_pos > size) new_pos = size;

    f->position = (uint16_t)new_pos;
    return new_pos;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use) return -1;
    vfs_state.fds[fd].in_use = false;
    vfs_state.fds[fd].fs_file = NULL;
    vfs_state.fds[fd].ops = NULL;
    return 0;
}

int vfs_stat(const char *path, vfs_stat_t *st) {
    if (!path || !st) return -1;
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) return -1;
    int result = vfs_fstat(fd, st);
    vfs_close(fd);
    return result;
}

int vfs_fstat(int fd, vfs_stat_t *st) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !vfs_state.fds[fd].in_use || !st) return -1;
    vfs_fd_t *f = &vfs_state.fds[fd];
    st->size = f->ops->size(f->fs_file);
    st->type = 0;
    st->flags = 0;
    return 0;
}

void vfs_get_stats(vfs_stats_t *stats) {
    if (!stats) return;
    stats->mounts_total = VFS_MAX_MOUNTS;
    stats->fds_total = VFS_MAX_FDS;
    stats->mounts_used = 0;
    stats->fds_used = 0;
    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++)
        if (vfs_state.mounts[i].type != VFS_TYPE_NONE) stats->mounts_used++;
    for (int i = 0; i < VFS_MAX_FDS; i++)
        if (vfs_state.fds[i].in_use) stats->fds_used++;
}

void vfs_print_mounts(void) {}

#endif /* CONFIG_FS_ENABLED */
