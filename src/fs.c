#include "fs.h"
#include <string.h>

/** Simple in-memory disk image. Each block is \c FS_BLOCK_SIZE bytes. */
static uint8_t disk[FS_NUM_BLOCKS][FS_BLOCK_SIZE];

/** Bitmap tracking used blocks. One bit per block. */
static uint8_t bitmap[FS_NUM_BLOCKS / 8];

/** Array of inodes. inode 0 is reserved for the root directory. */
static dinode_t inodes[FS_NUM_INODES];

/** Simple flat directory mapping inode numbers to filenames. */
static char dir_name[FS_NUM_INODES][FS_MAX_NAME + 1];

/** Helper: allocate a free block. Returns block number or -1. */
static int balloc(void) {
    for (uint8_t i = 0; i < FS_NUM_BLOCKS; ++i) {
        uint8_t byte = bitmap[i >> 3];
        uint8_t mask = 1u << (i & 7);
        if (!(byte & mask)) {
            bitmap[i >> 3] |= mask;
            memset(disk[i], 0, FS_BLOCK_SIZE);
            return i;
        }
    }
    return -1;
}


/** Helper: allocate a free inode. Returns inode number or -1. */
static int ialloc(uint8_t type) {
    for (uint8_t i = 0; i < FS_NUM_INODES; ++i) {
        if (inodes[i].type == 0) {
            inodes[i].type = type;
            inodes[i].nlink = 1;
            inodes[i].size = 0;
            memset(inodes[i].addrs, 0, sizeof inodes[i].addrs);
            return i;
        }
    }
    return -1;
}

void fs_init(void) {
    memset(disk, 0, sizeof disk);
    memset(bitmap, 0, sizeof bitmap);
    memset(inodes, 0, sizeof inodes);
    memset(dir_name, 0, sizeof dir_name);

    /* Reserve inode 0 as the root directory. */
    inodes[0].type = 2;  /* directory */
    strcpy(dir_name[0], "/");
}

int fs_create(const char *name, uint8_t type) {
    int inum = ialloc(type);
    if (inum < 0) {
        return -1;
    }
    strncpy(dir_name[inum], name, FS_MAX_NAME);
    dir_name[inum][FS_MAX_NAME] = '\0';
    return inum;
}

int fs_open(const char *name, file_t *f) {
    for (uint8_t i = 0; i < FS_NUM_INODES; ++i) {
        if (inodes[i].type && strncmp(dir_name[i], name, FS_MAX_NAME) == 0) {
            f->inum = i;
            f->off = 0;
            return 0;
        }
    }
    return -1;
}

int fs_write(file_t *f, const void *buf, uint16_t len) {
    dinode_t *d = &inodes[f->inum];
    const uint8_t *p = (const uint8_t *)buf;
    uint16_t remaining = len;

    while (remaining > 0) {
        uint8_t block_index = f->off / FS_BLOCK_SIZE;
        if (block_index >= (sizeof d->addrs / sizeof d->addrs[0])) {
            return len - remaining; /* no space for indirect blocks */
        }
        if (d->addrs[block_index] == 0) {
            int b = balloc();
            if (b < 0) {
                return len - remaining; /* out of blocks */
            }
            d->addrs[block_index] = b;
        }
        uint8_t *blk = disk[d->addrs[block_index]];
        uint16_t off_in_block = f->off % FS_BLOCK_SIZE;
        uint16_t to_copy = FS_BLOCK_SIZE - off_in_block;
        if (to_copy > remaining) {
            to_copy = remaining;
        }
        memcpy(blk + off_in_block, p, to_copy);
        f->off += to_copy;
        p += to_copy;
        remaining -= to_copy;
    }
    if (f->off > d->size) {
        d->size = f->off;
    }
    return len;
}

int fs_read(file_t *f, void *buf, uint16_t len) {
    dinode_t *d = &inodes[f->inum];
    uint8_t *p = (uint8_t *)buf;
    uint16_t remaining = len;

    if (f->off >= d->size) {
        return 0;
    }
    if (len > d->size - f->off) {
        remaining = d->size - f->off;
    }

    uint16_t read_len = remaining;
    while (remaining > 0) {
        uint8_t block_index = f->off / FS_BLOCK_SIZE;
        if (block_index >= (sizeof d->addrs / sizeof d->addrs[0])) {
            break;
        }
        if (d->addrs[block_index] == 0) {
            break;
        }
        uint8_t *blk = disk[d->addrs[block_index]];
        uint16_t off_in_block = f->off % FS_BLOCK_SIZE;
        uint16_t to_copy = FS_BLOCK_SIZE - off_in_block;
        if (to_copy > remaining) {
            to_copy = remaining;
        }
        memcpy(p, blk + off_in_block, to_copy);
        f->off += to_copy;
        p += to_copy;
        remaining -= to_copy;
    }
    return read_len - remaining;
}

/**
 * Copy the names of all valid inodes into ``out``.
 *
 * The directory is flat so at most ``FS_NUM_INODES`` entries exist. The caller
 * must provide storage for that many strings of length ``FS_MAX_NAME`` + 1.
 *
 * \param[out] out  Destination array of strings.
 * \return          Number of entries copied.
 */
int fs_list(char (*out)[FS_MAX_NAME + 1]) {
    if (out == NULL) {
        return 0;
    }

    int count = 0;
    for (uint8_t i = 0; i < FS_NUM_INODES; ++i) {
        if (inodes[i].type != 0) {
            strncpy(out[count], dir_name[i], FS_MAX_NAME);
            out[count][FS_MAX_NAME] = '\0';
            ++count;
        }
    }

    return count;
}
