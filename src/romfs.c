/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "romfs.h"
#include <string.h>
#include <avr/pgmspace.h>

#define ARRAY_LEN(x) ((sizeof(x) / sizeof((x)[0])))

/* ----------------------------------------------------------------------
 * ROMFS descriptor layout
 * ------------------------------------------------------------------ */
#define ROMFS_FILE 1u
#define ROMFS_DIR  2u

typedef struct {
    const char    *name;  /* pointer to PROGMEM string */
    uint8_t        type;  /* ROMFS_FILE or ROMFS_DIR   */
    uint8_t        idx;   /* index into dir_table or file_table */
} romfs_entry_t;

typedef struct {
    const romfs_entry_t *entries; /* pointer to PROGMEM array */
    uint8_t              count;   /* number of entries        */
} romfs_dir_t;

/* ----------------------------------------------------------------------
 * Sample 3-level hierarchy
 * ------------------------------------------------------------------ */
static const uint8_t ver_txt[] PROGMEM = "1.0\n";
static const uint8_t readme_txt[] PROGMEM = "ROMFS demo\n";

static const char name_ver[]    PROGMEM = "version.txt";
static const char name_cfg[]    PROGMEM = "config";
static const char name_etc[]    PROGMEM = "etc";
static const char name_readme[] PROGMEM = "README";

static const romfs_file_t file_table[] PROGMEM = {
    { ver_txt, sizeof ver_txt - 1 },
    { readme_txt, sizeof readme_txt - 1 }
};

static const romfs_entry_t config_entries[] PROGMEM = {
    { name_ver, ROMFS_FILE, 0 }
};
static const romfs_dir_t config_dir PROGMEM = { config_entries, 1 };

static const romfs_entry_t etc_entries[] PROGMEM = {
    { name_cfg, ROMFS_DIR, 0 }
};
static const romfs_dir_t etc_dir PROGMEM = { etc_entries, 1 };

static const romfs_entry_t root_entries[] PROGMEM = {
    { name_etc, ROMFS_DIR, 1 },
    { name_readme, ROMFS_FILE, 1 }
};
static const romfs_dir_t root_dir PROGMEM = { root_entries, 2 };

static const romfs_dir_t dir_table[] PROGMEM = {
    config_dir, /* 0 */
    etc_dir,    /* 1 */
    root_dir    /* 2 */
};

#define ROOT_DIR (&dir_table[2])

/* ----------------------------------------------------------------------
 * Helper: compare a RAM string to one in flash
 * ------------------------------------------------------------------ */
static bool equal_p(const char *s, const char *pstr)
{
    char c;
    while ((c = *s++)) {
        if (c != pgm_read_byte(pstr++))
            return false;
    }
    return pgm_read_byte(pstr) == 0;
}

/* ----------------------------------------------------------------------
 * romfs_open: walk path from root, return file descriptor
 * ------------------------------------------------------------------ */
const romfs_file_t *romfs_open(const char *path)
{
    const romfs_dir_t *dir = ROOT_DIR;
    const char *p = path;
    if (*p == '/')
        p++;
    char seg[12];
    while (*p) {
        size_t n = 0;
        while (p[n] && p[n] != '/' && n < sizeof seg - 1) {
            seg[n] = p[n];
            n++;
        }
        seg[n] = '\0';
        p += n;
        if (*p == '/')
            p++;
        uint8_t count = pgm_read_byte(&dir->count);
        romfs_entry_t entry;
        const romfs_entry_t *entries;
#ifdef __AVR__
        /* Directory tables are stored in program memory */
        entries = (const romfs_entry_t *)pgm_read_word(&dir->entries);
#else
        /* Host build: arrays already reside in RAM */
        entries = dir->entries;
#endif
        bool found = false;
        for (uint8_t i = 0; i < count; i++) {
            memcpy_P(&entry, &entries[i], sizeof entry);
            const char *nm;
#ifdef __AVR__
            /* Entry names stored in flash require pgm_read_word */
            nm = (const char *)pgm_read_word(&entry.name);
#else
            nm = entry.name;
#endif
            if (equal_p(seg, nm)) {
                found = true;
                if (*p == '\0' && entry.type == ROMFS_FILE) {
                    if (entry.idx >= ARRAY_LEN(file_table))
                        return NULL;
                    return &file_table[entry.idx];
                } else if (entry.type == ROMFS_DIR) {
                    if (entry.idx >= ARRAY_LEN(dir_table))
                        return NULL;
                    dir = &dir_table[entry.idx];
                } else {
                    return NULL;
                }
                break;
            }
        }
        if (!found)
            return NULL;
    }
    return NULL;
}

/* ----------------------------------------------------------------------
 * romfs_read: copy bytes from flash into RAM buffer
 * ------------------------------------------------------------------ */
int romfs_read(const romfs_file_t *f, uint16_t off, void *buf, uint16_t len)
{
    romfs_file_t tmp;
    memcpy_P(&tmp, f, sizeof tmp);
    if (off >= tmp.size)
        return 0;
    if (off + len > tmp.size)
        len = tmp.size - off;
    memcpy_P(buf, tmp.data + off, len);
    return len;
}
