/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file eepfs.c
 * @brief Portable EEPROM Filesystem Implementation
 *
 * Read/write filesystem stored in EEPROM using HAL abstractions.
 * Optimized for minimal wear with update-only writes.
 */

#include "eepfs.h"
#include "arch/common/hal.h"
#include <string.h>

#define ARRAY_LEN(x) ((sizeof(x) / sizeof((x)[0])))

/*═══════════════════════════════════════════════════════════════════
 * EEPFS DESCRIPTOR LAYOUT
 *═══════════════════════════════════════════════════════════════════*/

#define EEPFS_FILE 1u
#define EEPFS_DIR  2u

/**
 * @brief Directory entry (stored in program memory)
 *
 * Each entry describes either a file or a subdirectory.
 */
typedef struct {
    const char *name;  /**< Pointer to name string in program memory */
    uint8_t type;      /**< EEPFS_FILE or EEPFS_DIR */
    uint8_t idx;       /**< Index into dir_table or file_table */
} eepfs_entry_t;

/**
 * @brief Directory descriptor (stored in program memory)
 */
typedef struct {
    const eepfs_entry_t *entries;  /**< Pointer to entry array in program memory */
    uint8_t count;                 /**< Number of entries in this directory */
} eepfs_dir_t;

/*═══════════════════════════════════════════════════════════════════
 * EEPROM LAYOUT
 *═══════════════════════════════════════════════════════════════════
 * Address 0x0000: File 0 data (message.txt)
 * Address 0x000A: (future files...)
 *═══════════════════════════════════════════════════════════════════*/

/* Initial file content for EEPROM (written during format) */
static const uint8_t initial_message[] HAL_PROGMEM = "EEPROM FS\n";

#define FILE0_ADDR 0x0000
#define FILE0_SIZE (sizeof(initial_message) - 1)

/* Entry names (stored in program memory) */
static const char name_sys[] HAL_PROGMEM = "sys";
static const char name_msg[] HAL_PROGMEM = "message.txt";

/* File table (stored in program memory) */
static const eepfs_file_t file_table[] HAL_PROGMEM = {
    { FILE0_ADDR, FILE0_SIZE }  /* /sys/message.txt */
};

/* Directory: /sys/ */
static const eepfs_entry_t sys_entries[] HAL_PROGMEM = {
    { name_msg, EEPFS_FILE, 0 }
};
static const eepfs_dir_t sys_dir HAL_PROGMEM = { sys_entries, 1 };

/* Directory: / (root) */
static const eepfs_entry_t root_entries[] HAL_PROGMEM = {
    { name_sys, EEPFS_DIR, 0 }
};
static const eepfs_dir_t root_dir HAL_PROGMEM = { root_entries, 1 };

/* Directory table (stored in program memory) */
static const eepfs_dir_t dir_table[] HAL_PROGMEM = {
    sys_dir,   /* 0 */
    root_dir   /* 1 */
};

#define ROOT_DIR (&dir_table[1])

/*═══════════════════════════════════════════════════════════════════
 * HELPER: COMPARE RAM STRING TO FLASH STRING
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Compare a RAM string to one in flash
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
 */
const eepfs_file_t *eepfs_open(const char *path) {
    const eepfs_dir_t *dir = ROOT_DIR;
    const char *p = path;

    /* Skip leading '/' */
    if (*p == '/') {
        p++;
    }

    char seg[12];  /* Path segment buffer */

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
        eepfs_entry_t entry;
        const eepfs_entry_t *entries;

        /* Read entries pointer from program memory */
        entries = (const eepfs_entry_t *)hal_pgm_read_word(&dir->entries);

        bool found = false;
        for (uint8_t i = 0; i < count; i++) {
            /* Copy entry from program memory to RAM */
            hal_memcpy_P(&entry, &entries[i], sizeof(entry));

            /* Read entry name pointer from program memory */
            const char *nm = (const char *)hal_pgm_read_word(&entry.name);

            if (equal_p(seg, nm)) {
                found = true;

                /* If this is the last segment and it's a file, return it */
                if (*p == '\0' && entry.type == EEPFS_FILE) {
                    if (entry.idx >= ARRAY_LEN(file_table)) {
                        return NULL;
                    }
                    return &file_table[entry.idx];
                }
                /* If it's a directory, descend into it */
                else if (entry.type == EEPFS_DIR) {
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
 * @brief Read from an EEPFS file
 */
int eepfs_read(const eepfs_file_t *f, uint16_t off, void *buf, uint16_t len) {
    eepfs_file_t tmp;

    /* Copy file descriptor from program memory to RAM */
    hal_memcpy_P(&tmp, f, sizeof(tmp));

    /* Check bounds */
    if (off >= tmp.size) {
        return 0;  /* Offset past end of file */
    }
    if (off + len > tmp.size) {
        len = tmp.size - off;  /* Truncate to EOF */
    }

    /* Read from EEPROM using HAL */
    hal_eeprom_read_block(buf, tmp.addr + off, len);

    return len;
}

/**
 * @brief Write to an EEPFS file
 *
 * Uses update semantics to minimize EEPROM wear.
 */
int eepfs_write(const eepfs_file_t *f, uint16_t off, const void *buf, uint16_t len) {
    eepfs_file_t tmp;

    /* Copy file descriptor from program memory to RAM */
    hal_memcpy_P(&tmp, f, sizeof(tmp));

    /* Check bounds */
    if (off >= tmp.size) {
        return 0;  /* Cannot extend file */
    }
    if (off + len > tmp.size) {
        len = tmp.size - off;  /* Truncate to EOF */
    }

    /* Write to EEPROM using update (only writes changed bytes) */
    hal_eeprom_update_block(tmp.addr + off, buf, len);

    return len;
}

/**
 * @brief Format EEPROM with initial filesystem structure
 */
void eepfs_format(void) {
    /* Check if EEPROM is available */
    if (!hal_eeprom_available()) {
        return;  /* No EEPROM on this platform */
    }

    /* Write initial file content to EEPROM */
    /* Using update to avoid unnecessary writes if already formatted */
    hal_eeprom_update_block(FILE0_ADDR, initial_message, FILE0_SIZE);
}

/**
 * @brief Get EEPROM usage statistics
 */
void eepfs_stats(uint16_t *used_bytes, uint16_t *total_bytes) {
    if (used_bytes) {
        /* Calculate total bytes used by all files */
        uint16_t used = 0;
        for (uint8_t i = 0; i < ARRAY_LEN(file_table); i++) {
            eepfs_file_t f;
            hal_memcpy_P(&f, &file_table[i], sizeof(f));
            used += f.size;
        }
        *used_bytes = used;
    }

    if (total_bytes) {
        *total_bytes = hal_eeprom_size();
    }
}
