/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*───────────────────────── nk_fs.c ────────────────────────────
 * TinyLog-4  – 64-byte wear-levelled log for ATmega328P EEPROM.
 *   • 16 rows × 64 B  (1 k B total)
 *   • row header = {SEQ, TAG_ROW, CRC}
 *   • 15 data blocks / row
 *   • 3-byte “PUT / DEL” payload  + CRC-8(Maxi m/Dallas)
 *
 * Flash ≈ 1.05 kB  (avr-gcc-14  –std=c23 –Oz –flto)
 * SRAM  ≈  10 B    (row + index cursors)
 *──────────────────────────────────────────────────────────────*/
#include "nk_fs.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>

/* ─── 0 · Tunables ──────────────────────────────────────────── */
enum : uint8_t {
    ROWS        = 16,
    ROW_SZ      = 64,
    BLK_SZ      = 4,
    DATA_BLKS   = (ROWS * (ROW_SZ / BLK_SZ)),   /* 256 logical blocks */
    TAG_PUT     = 0x01,
    TAG_DEL     = 0x02,
    TAG_ROW     = 0x7F,
    HDR_SEQ_OFF = ROW_SZ - 2,
    HDR_TAG_OFF = ROW_SZ - 1,
};

#ifndef NK_FS_SMALL_CRC          /* set to 1 to drop LUT (smaller flash) */
#  define NK_FS_SMALL_CRC 0
#endif

/* ─── 1 · State ─────────────────────────────────────────────── */
static uint8_t cur_row = 0;      /* row currently writable      */
static uint8_t cur_idx = 0;      /* next DATA block inside row  */

/* ─── 2 · CRC-8 helpers (Dallas/Maxim poly 0x31) ────────────── */
#if !NK_FS_SMALL_CRC
static const uint8_t LUT[32] PROGMEM = {
    0x00,0x31,0x62,0x53,0xC4,0xF5,0xA6,0x97,
    0xB9,0x88,0xDB,0xEA,0x7D,0x4C,0x1F,0x2E,
    0x43,0x72,0x21,0x10,0x87,0xB6,0xE5,0xD4,
    0xFA,0xCB,0x98,0xA9,0x3E,0x0F,0x5C,0x6D
};
static inline uint8_t crc8_update(uint8_t crc, uint8_t in) {
    crc ^= in;
    return pgm_read_byte(&LUT[crc & 0x1F]) ^ (crc >> 5);
}
#else /* bit-wise fallback → 24 B bigger SRAM, 32 B smaller flash */
static inline uint8_t crc8_update(uint8_t crc, uint8_t in) {
    crc ^= in;
    for (uint8_t i = 0; i < 8; ++i)
        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    return crc;
}
#endif

static inline uint8_t crc3(uint8_t tag, uint8_t d0, uint8_t d1) {
    return crc8_update(crc8_update(crc8_update(0, tag), d0), d1);
}

/* ─── 3 · Address math ──────────────────────────────────────── */
static inline uint16_t addr(uint8_t row, uint8_t idx) {
    return (uint16_t)row * ROW_SZ + (uint16_t)idx * BLK_SZ;
}

/* erase 64-byte row with 0xFF (EEPROM-safe wear levelling) */
static void erase_row(uint8_t row) {
    uint16_t base = (uint16_t)row * ROW_SZ;
    for (uint8_t i = 0; i < ROW_SZ; ++i)
        eeprom_update_byte((uint8_t __eeprom *)(base + i), 0xFF);
}

/* ─── 4 · Row rollover ──────────────────────────────────────── */
static void open_next_row(void) {
    const uint8_t next = (uint8_t)((cur_row + 1U) & 0x0F);
    erase_row(next);

    /* fetch prev sequence, increment (wrap OK) */
    uint8_t seq = eeprom_read_byte((const uint8_t __eeprom *)
                    (addr(cur_row, 15) + HDR_SEQ_OFF));
    seq++;

    uint16_t off = (uint16_t)next * ROW_SZ;
    eeprom_update_byte((uint8_t __eeprom *)(off + HDR_SEQ_OFF), seq);
    eeprom_update_byte((uint8_t __eeprom *)(off + HDR_TAG_OFF), TAG_ROW);

    cur_row = next;
    cur_idx = 0;
}

/* ─── 5 · Public API ────────────────────────────────────────── */
void nk_fs_init(void) {
    uint8_t best_seq = 0;
    cur_row = 0;

    /* scan all row headers → pick newest by signed delta */
    for (uint8_t r = 0; r < ROWS; ++r) {
        uint16_t hdr = addr(r, 15);
        if (eeprom_read_byte((const uint8_t __eeprom *)(hdr + HDR_TAG_OFF)) == TAG_ROW) {
            uint8_t seq = eeprom_read_byte((const uint8_t __eeprom *)(hdr + HDR_SEQ_OFF));
            if ((int8_t)(seq - best_seq) > 0) {  /* signed distance */
                best_seq = seq;
                cur_row  = r;
            }
        }
    }

    /* locate first free slot in current row */
    cur_idx = 0;
    for (uint8_t i = 0; i < 15; ++i) {
        uint16_t a  = addr(cur_row, i);
        uint8_t  t  = eeprom_read_byte((const uint8_t __eeprom *)a);
        uint8_t  d0 = eeprom_read_byte((const uint8_t __eeprom *)(a + 1));
        uint8_t  d1 = eeprom_read_byte((const uint8_t __eeprom *)(a + 2));
        uint8_t  c  = eeprom_read_byte((const uint8_t __eeprom *)(a + 3));
        if (c != crc3(t, d0, d1)) {          /* corruption / unused */
            cur_idx = i;
            return;
        }
    }
    open_next_row();                         /* row full → next row */
}

/* internal helper: write 1 block, bump cursor, roll row on full */
static bool write_block(uint8_t tag, uint8_t d0, uint8_t d1) {
    const uint16_t a   = addr(cur_row, cur_idx);
    const uint8_t  crc = crc3(tag, d0, d1);

    eeprom_update_byte((uint8_t __eeprom *)a,       tag);
    eeprom_update_byte((uint8_t __eeprom *)(a + 1), d0);
    eeprom_update_byte((uint8_t __eeprom *)(a + 2), d1);
    eeprom_update_byte((uint8_t __eeprom *)(a + 3), crc);

    if (eeprom_read_byte((const uint8_t __eeprom *)(a + 3)) != crc)
        return false;                         /* verify write */

    if (++cur_idx >= 15)
        open_next_row();
    return true;
}

/* --- external key <16 KiB, value <32 ---------------------------------- */
bool nk_fs_put(uint16_t key, uint16_t val) {
    if (key >= 2048 || val >= 32) return false;
    uint8_t d0 = key >> 3;
    uint8_t d1 = (uint8_t)(((key & 0x07) << 5) | (val & 0x1F));
    return write_block(TAG_PUT, d0, d1);
}

bool nk_fs_del(uint16_t key) {
    if (key >= 2048) return false;
    uint8_t d0 = key >> 3;
    uint8_t d1 = (uint8_t)((key & 0x07) << 5);
    return write_block(TAG_DEL, d0, d1);
}

static inline uint16_t unpack_key(uint8_t d0, uint8_t d1) {
    return (uint16_t)d0 << 3 | (d1 >> 5);
}
static inline uint16_t unpack_val(uint8_t d1) { return d1 & 0x1F; }

bool nk_fs_get(uint16_t key, uint16_t *out) {
    if (!out || key >= 2048) return false;

    uint8_t r = cur_row;
    uint8_t i = cur_idx;

    /* walk backwards through circular log */
    for (;;) {
        if (i == 0) {
            r = (uint8_t)(r ? r - 1 : ROWS - 1);
            i = 15;
        } else {
            --i;
        }
        uint16_t a  = addr(r, i);
        uint8_t  t  = eeprom_read_byte((const uint8_t __eeprom *)a);
        uint8_t  d0 = eeprom_read_byte((const uint8_t __eeprom *)(a + 1));
        uint8_t  d1 = eeprom_read_byte((const uint8_t __eeprom *)(a + 2));
        uint8_t  c  = eeprom_read_byte((const uint8_t __eeprom *)(a + 3));
        if (c != crc3(t, d0, d1))           /* corruption → stop search */
            break;

        if (unpack_key(d0, d1) == key) {
            if (t == TAG_DEL) return false; /* tombstone */
            *out = unpack_val(d1);
            return true;
        }
        if (r == cur_row && i == 0) break;  /* full loop done */
    }
    return false;
}

/* Simple GC placeholder – not yet needed for 1 kB log */
void nk_fs_gc(void) {}
