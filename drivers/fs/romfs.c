/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file romfs.c
 * @brief Portable ROMFS Implementation
 *
 * Read-only filesystem stored in program flash/ROM using HAL abstractions.
 */

#include "romfs.h"
#include "arch/common/hal.h"
#include <string.h>

#define ARRAY_LEN(x) ((sizeof(x) / sizeof((x)[0])))

/*═══════════════════════════════════════════════════════════════════
 * ROMFS DESCRIPTOR LAYOUT
 *═══════════════════════════════════════════════════════════════════*/

#define ROMFS_FILE 1u
#define ROMFS_DIR  2u

/**
 * @brief Directory entry (stored in program memory)
 *
 * Each entry describes either a file or a subdirectory.
 * Size: 4 bytes on 8/16-bit, 6+ bytes on 32-bit (pointer size dependent).
 */
typedef struct {
    const char *name;  /**< Pointer to name string in program memory */
    uint8_t type;      /**< ROMFS_FILE or ROMFS_DIR */
    uint8_t idx;       /**< Index into dir_table or file_table */
} romfs_entry_t;

/**
 * @brief Directory descriptor (stored in program memory)
 *
 * Describes a directory and its entries.
 */
typedef struct {
    const romfs_entry_t *entries;  /**< Pointer to entry array in program memory */
    uint8_t count;                 /**< Number of entries in this directory */
} romfs_dir_t;

/*═══════════════════════════════════════════════════════════════════
 * SAMPLE FILESYSTEM (3-LEVEL HIERARCHY)
 *═══════════════════════════════════════════════════════════════════
 * /
 * ├── etc/
 * │   └── config/
 * │       └── version.txt
 * └── README
 *═══════════════════════════════════════════════════════════════════*/

/* File data (stored in program memory) */
static const uint8_t ver_txt[] HAL_PROGMEM = "1.0\n";
static const uint8_t readme_txt[] HAL_PROGMEM = "ROMFS demo\n";

/* Entry names (stored in program memory) */
static const char name_ver[] HAL_PROGMEM = "version.txt";
static const char name_cfg[] HAL_PROGMEM = "config";
static const char name_etc[] HAL_PROGMEM = "etc";
static const char name_readme[] HAL_PROGMEM = "README";

/* File table (stored in program memory) */
static const romfs_file_t file_table[] HAL_PROGMEM = {
    { ver_txt, sizeof(ver_txt) - 1 },
    { readme_txt, sizeof(readme_txt) - 1 }
};

/* Directory: /etc/config/ */
static const romfs_entry_t config_entries[] HAL_PROGMEM = {
    { name_ver, ROMFS_FILE, 0 }
};
static const romfs_dir_t config_dir HAL_PROGMEM = { config_entries, 1 };

/* Directory: /etc/ */
static const romfs_entry_t etc_entries[] HAL_PROGMEM = {
    { name_cfg, ROMFS_DIR, 0 }
};
static const romfs_dir_t etc_dir HAL_PROGMEM = { etc_entries, 1 };

/* Directory: / (root) */
static const romfs_entry_t root_entries[] HAL_PROGMEM = {
    { name_etc, ROMFS_DIR, 1 },
    { name_readme, ROMFS_FILE, 1 }
};
static const romfs_dir_t root_dir HAL_PROGMEM = { root_entries, 2 };

/* Directory table (stored in program memory) */
static const romfs_dir_t dir_table[] HAL_PROGMEM = {
    config_dir, /* 0 */
    etc_dir,    /* 1 */
    root_dir    /* 2 */
};

#define ROOT_DIR (&dir_table[2])

/*═══════════════════════════════════════════════════════════════════
 * HELPER: COMPARE RAM STRING TO FLASH STRING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Compare a RAM string to one in flash
 *
 * @param s String in RAM
 * @param pstr String in program memory (flash)
 * @return true if strings are equal, false otherwise
 */
static bool equal_p(const char *s, const char *pstr) {
    char c;
    while ((c = *s++)) {
        if (c != hal_pgm_read_byte(pstr++)) {
            return false;
        }
    }
    return hal_pgm_read_byte(pstr) == 0;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API IMPLEMENTATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Open a file by absolute path
 *
 * Walks the directory tree from root, resolving each path component.
 *
 * @param path Absolute UNIX-style path
 * @return Pointer to file descriptor, or NULL if not found
 */
const romfs_file_t *romfs_open(const char *path) {
    const romfs_dir_t *dir = ROOT_DIR;
    const char *p = path;

    /* Skip leading '/' */
    if (*p == '/') {
        p++;
    }

    char seg[12];  /* Path segment buffer (max 11 chars + null) */

    while (*p) {
        /* Extract next path segment */
        size_t n = 0;
        while (p[n] && p[n] != '/' && n < sizeof(seg) - 1) {
            seg[n] = p[n];
            n++;
        }
        seg[n] = '\0';
        p += n;
        if (*p == '/') {
            p++;
        }

        /* Search current directory for segment */
        uint8_t count = hal_pgm_read_byte(&dir->count);
        romfs_entry_t entry;
        const romfs_entry_t *entries;

        /* Read entries pointer from program memory */
        entries = (const romfs_entry_t *)hal_pgm_read_word(&dir->entries);

        bool found = false;
        for (uint8_t i = 0; i < count; i++) {
            /* Copy entry from program memory to RAM */
            hal_memcpy_P(&entry, &entries[i], sizeof(entry));

            /* Read entry name pointer from program memory */
            const char *nm = (const char *)hal_pgm_read_word(&entry.name);

            if (equal_p(seg, nm)) {
                found = true;

                /* If this is the last segment and it's a file, return it */
                if (*p == '\0' && entry.type == ROMFS_FILE) {
                    if (entry.idx >= ARRAY_LEN(file_table)) {
                        return NULL;
                    }
                    return &file_table[entry.idx];
                }
                /* If it's a directory, descend into it */
                else if (entry.type == ROMFS_DIR) {
                    if (entry.idx >= ARRAY_LEN(dir_table)) {
                        return NULL;
                    }
                    dir = &dir_table[entry.idx];
                }
                /* Path component is a file but not the last segment */
                else {
                    return NULL;
                }
                break;
            }
        }

        if (!found) {
            return NULL;  /* Path component not found */
        }
    }

    return NULL;  /* Path ended in directory, not file */
}

/**
 * @brief Read from a ROMFS file
 *
 * Copies data from flash/ROM into a RAM buffer.
 *
 * @param f Pointer to file descriptor
 * @param off Offset into file (bytes)
 * @param buf Destination buffer in RAM
 * @param len Number of bytes to read
 * @return Number of bytes actually read
 */
int romfs_read(const romfs_file_t *f, uint16_t off, void *buf, uint16_t len) {
    romfs_file_t tmp;

    /* Copy file descriptor from program memory to RAM */
    hal_memcpy_P(&tmp, f, sizeof(tmp));

    /* Check bounds */
    if (off >= tmp.size) {
        return 0;  /* Offset past end of file */
    }
    if (off + len > tmp.size) {
        len = tmp.size - off;  /* Truncate to EOF */
    }

    /* Copy file data from program memory to RAM */
    hal_memcpy_P(buf, tmp.data + off, len);

    return len;
}
