/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "eepfs.h"
#include <string.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#define ARRAY_LEN(x) ((sizeof((x)) / sizeof((x)[0])))

#define EEPFS_FILE 1u
#define EEPFS_DIR  2u

/** Entry within an EEPROM directory table. */
typedef struct {
    const char *name;  /* RAM -> name string in flash */
    uint8_t     type;  /* file or dir                */
    uint8_t     idx;   /* index into tables          */
} eepfs_entry_t;

/** Directory descriptor containing a list of entries. */
typedef struct {
    const eepfs_entry_t *entries;
    uint8_t              count;
} eepfs_dir_t;

/* Example file contents */
static const uint8_t eem_ver_txt[] EEMEM = "eeprom fs\n";
static const char name_sys[] PROGMEM = "sys";
static const char name_msg[] PROGMEM = "message.txt";

static const eepfs_file_t file_tab[] PROGMEM = {
    { 0, 9 } /* ver file */
};

static const eepfs_entry_t sys_entries[] PROGMEM = {
    { name_msg, EEPFS_FILE, 0 }
};
static const eepfs_dir_t sys_dir PROGMEM = { sys_entries, 1 };

static const eepfs_entry_t root_entries[] PROGMEM = {
    { name_sys, EEPFS_DIR, 0 }
};
static const eepfs_dir_t root_dir PROGMEM = { root_entries, 1 };

static const eepfs_dir_t dir_tab[] PROGMEM = {
    sys_dir,
    root_dir
};

#define ROOT_DIR (&dir_tab[1])

static bool eq_p(const char *s, const char *p)
{
    char c;
    while ((c = *s++)) {
        if (c != pgm_read_byte(p++))
            return false;
    }
    return pgm_read_byte(p) == 0;
}

const eepfs_file_t *eepfs_open(const char *path)
{
    const eepfs_dir_t *dir = ROOT_DIR;
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
        eepfs_entry_t ent;
        const eepfs_entry_t *ents = (const eepfs_entry_t *)pgm_read_word(&dir->entries);
        bool found = false;
        for (uint8_t i = 0; i < count; i++) {
            memcpy_P(&ent, &ents[i], sizeof ent);
            const char *nm = (const char *)pgm_read_word(&ent.name);
            if (eq_p(seg, nm)) {
                found = true;
                if (*p == '\0' && ent.type == EEPFS_FILE) {
                    if (ent.idx >= ARRAY_LEN(file_tab))
                        return NULL;
                    return &file_tab[ent.idx];
                } else if (ent.type == EEPFS_DIR) {
                    if (ent.idx >= ARRAY_LEN(dir_tab))
                        return NULL;
                    dir = &dir_tab[ent.idx];
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

int eepfs_read(const eepfs_file_t *f, uint16_t off, void *buf, uint16_t len)
{
    eepfs_file_t tmp;
    memcpy_P(&tmp, f, sizeof tmp);
    if (off >= tmp.size)
        return 0;
    if (off + len > tmp.size)
        len = tmp.size - off;
    for (uint16_t i = 0; i < len; i++) {
        ((uint8_t *)buf)[i] = eeprom_read_byte((const uint8_t *)(uintptr_t)(tmp.addr + off + i));
    }
    return len;
}
