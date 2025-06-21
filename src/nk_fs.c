#include "nk_fs.h"
#include <stdbool.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "eeprom_wrap.h"

/* TinyLog-4 layout constants */
#define NK_ROWS       16u
#define NK_ROW_SIZE   64u
#define NK_BLOCK_SIZE 4u
#define NK_BLOCKS     ((NK_ROWS) * ((NK_ROW_SIZE) / (NK_BLOCK_SIZE)))

/* Record tags */
#define TAG_PUT   0x01u
#define TAG_DEL   0x02u
#define TAG_ROW   0x7Fu

/* Row header offset from start of row */
#define ROW_SEQ_OFF   (NK_ROW_SIZE - 2)
#define ROW_TAG_OFF   (NK_ROW_SIZE - 1)

/* Cursor location of next write */
static uint8_t nk_row;
static uint8_t nk_idx;

/* Dallas/Maxim CRC-8 table (0x31 polynomial) */
static const uint8_t crc_tbl[32] PROGMEM = {
    0x00,0x31,0x62,0x53,0xC4,0xF5,0xA6,0x97,
    0xB9,0x88,0xDB,0xEA,0x7D,0x4C,0x1F,0x2E,
    0x43,0x72,0x21,0x10,0x87,0xB6,0xE5,0xD4,
    0xFA,0xCB,0x98,0xA9,0x3E,0x0F,0x5C,0x6D
};

static uint8_t crc8_update(uint8_t crc, uint8_t data) {
    crc ^= data;
    crc = pgm_read_byte(&crc_tbl[crc & 0x1F]) ^ (crc >> 5);
    return crc;
}

static uint8_t calc_crc(uint8_t t, uint8_t d0, uint8_t d1) {
    uint8_t c = 0;
    c = crc8_update(c, t);
    c = crc8_update(c, d0);
    c = crc8_update(c, d1);
    return c;
}

static uint16_t addr_for(uint8_t row, uint8_t idx) {
    return (uint16_t)row * NK_ROW_SIZE + (uint16_t)idx * NK_BLOCK_SIZE;
}

static void erase_row(uint8_t row) {
    uint16_t base = (uint16_t)row * NK_ROW_SIZE;
    for (uint8_t i = 0; i < NK_ROW_SIZE; ++i)
        eeprom_update_byte(ee_ptr((uint16_t)(base + i)), 0xFF);
}

static void open_next_row(void) {
    uint8_t next = (nk_row + 1) & 0x0F;
    erase_row(next);
    uint8_t seq = eeprom_read_byte(ee_cptr(addr_for(nk_row, 15) + ROW_SEQ_OFF));
    seq++;
    eeprom_update_byte(ee_ptr((uint16_t)(next * NK_ROW_SIZE + ROW_SEQ_OFF)), seq);
    eeprom_update_byte(ee_ptr((uint16_t)(next * NK_ROW_SIZE + ROW_TAG_OFF)), TAG_ROW);
    nk_row = next;
    nk_idx = 0;
}

void nk_fs_init(void) {
    uint8_t latest_seq = 0;
    nk_row = 0;
    for (uint8_t r = 0; r < NK_ROWS; ++r) {
        uint16_t hdr = addr_for(r, 15);
        if (eeprom_read_byte(ee_cptr(hdr + 3)) == TAG_ROW) {
            uint8_t seq = eeprom_read_byte(ee_cptr(hdr + 2));
            if ((int8_t)(seq - latest_seq) > 0) {
                latest_seq = seq;
                nk_row = r;
            }
        }
    }
    nk_idx = 0;
    for (uint8_t i = 0; i < 15; ++i) {
        uint16_t a = addr_for(nk_row, i);
        uint8_t t = eeprom_read_byte(ee_cptr(a));
        uint8_t d0 = eeprom_read_byte(ee_cptr(a + 1));
        uint8_t d1 = eeprom_read_byte(ee_cptr(a + 2));
        uint8_t c  = eeprom_read_byte(ee_cptr(a + 3));
        if (c != calc_crc(t, d0, d1)) {
            nk_idx = i;
            return;
        }
    }
    open_next_row();
}

static bool write_rec(uint8_t tag, uint8_t d0, uint8_t d1) {
    uint16_t a = addr_for(nk_row, nk_idx);
    uint8_t crc = calc_crc(tag, d0, d1);
    eeprom_update_byte(ee_ptr(a), tag);
    eeprom_update_byte(ee_ptr(a + 1), d0);
    eeprom_update_byte(ee_ptr(a + 2), d1);
    eeprom_update_byte(ee_ptr(a + 3), crc);
    if (eeprom_read_byte(ee_cptr(a + 3)) != crc)
        return false;
    nk_idx++;
    if (nk_idx >= 15)
        open_next_row();
    return true;
}

bool nk_fs_put(uint16_t key, uint16_t val) {
    if (key >= 2048)
        return false;
    uint8_t d0 = key >> 3;
    uint8_t d1 = ((key & 7) << 5) | (val & 0x1F);
    return write_rec(TAG_PUT, d0, d1);
}

bool nk_fs_del(uint16_t key) {
    if (key >= 2048)
        return false;
    uint8_t d0 = key >> 3;
    uint8_t d1 = (key & 7) << 5;
    return write_rec(TAG_DEL, d0, d1);
}

static uint16_t extract_key(uint8_t d0, uint8_t d1) {
    return ((uint16_t)d0 << 3) | (d1 >> 5);
}

static uint16_t extract_val(uint8_t d1) {
    return d1 & 0x1F;
}

bool nk_fs_get(uint16_t key, uint16_t *val) {
    uint8_t r = nk_row;
    uint8_t i = nk_idx;
    do {
        if (i == 0) {
            r = (r == 0)
                ? (uint8_t)(NK_ROWS - 1)
                : (uint8_t)(r - 1);
            i = 15;
        } else {
            i--;
        }
        uint16_t a = addr_for(r, i);
        uint8_t t  = eeprom_read_byte(ee_cptr(a));
        uint8_t d0 = eeprom_read_byte(ee_cptr(a + 1));
        uint8_t d1 = eeprom_read_byte(ee_cptr(a + 2));
        uint8_t c  = eeprom_read_byte(ee_cptr(a + 3));
        if (c != calc_crc(t, d0, d1))
            break;
        uint16_t k = extract_key(d0, d1);
        if (k == key) {
            if (t == TAG_DEL)
                return false;
            *val = extract_val(d1);
            return true;
        }
    } while (r != nk_row || i != nk_idx);
    return false;
}

void nk_fs_gc(void) {
    /* Simple GC: not implemented for brevity. */
}

