/*────────────────────────── door.c ────────────────────────────
   Zero-copy Door/Cap’n-Proto RPC layer for µ-UNIX on ATmega328P.
   Foot-print  ≈  740 B flash / 3 B SRAM.
   ─────────────────────────────────────────────────────────────*/

#include "door.h"
#include "nk_task.h"
#include <avr/pgmspace.h>
#include <string.h>

/*────────────────── persistent state (.noinit) ───────────────*/
uint8_t door_slab[DOOR_SLAB_SIZE]        __attribute__((section(".noinit")));

static door_t   door_vec[NK_MAX_TASKS][DOOR_SLOTS]
                                  __attribute__((section(".noinit")));

static volatile uint8_t door_caller       __attribute__((section(".noinit")));
static volatile uint8_t door_msg_words;
static volatile uint8_t door_msg_flags;

/*────────────────── local helpers ────────────────────────────*/
/* small CRC-8 LUT (Dallas/Maxim 0x31) – 32 B flash */
static uint8_t crc8_calc(const uint8_t *p, uint8_t len)
{
    static const uint8_t lut[32] PROGMEM = {
        0x00,0x31,0x62,0x53,0xC4,0xF5,0xA6,0x97,
        0xB9,0x88,0xDB,0xEA,0x7D,0x4C,0x1F,0x2E,
        0x43,0x72,0x21,0x10,0x87,0xB6,0xE5,0xD4,
        0xFA,0xCB,0x98,0xA9,0x3E,0x0F,0x5C,0x6D
    };
    uint8_t crc = 0;
    while (len--) {
        uint8_t in = *p++ ^ crc;
        crc = (crc << 5) ^ pgm_read_byte(&lut[in >> 3]) ^ (in & 7);
    }
    return crc;
}

/*────────────────── public API ───────────────────────────────*/
void door_register(uint8_t idx, uint8_t target,
                   uint8_t words, uint8_t flags)
{
    if (idx >= DOOR_SLOTS || words == 0 ||
        words * 8u > DOOR_SLAB_SIZE) return;

    door_vec[nk_cur_tid()][idx] =
        (door_t){ .tgt_tid = target,
                  .words   = (uint8_t)(words & 0x0F),
                  .flags   = (uint8_t)(flags & 0x0F) };
}

void door_call(uint8_t idx, void *buf /*↔ reply*/)
{
    uint8_t caller = nk_cur_tid();
    if (idx >= DOOR_SLOTS) return;

    door_t d = door_vec[caller][idx];
    if (d.words == 0) return;                    /* slot empty        */

    uint8_t bytes = (uint8_t)(d.words * 8u);
    memcpy(door_slab, buf, bytes);               /* copy request      */

    if (d.flags & 0x01)                          /* optional CRC-8    */
        door_slab[bytes] = crc8_calc(door_slab, bytes);

    door_caller       = caller;
    door_msg_words    = d.words;
    door_msg_flags    = d.flags;

    nk_switch_to(d.tgt_tid);                     /* ↔ callee          */

    memcpy(buf, door_slab, bytes);               /* fetch reply       */
}

void door_return(void)                           /* callee side */
{
    nk_switch_to(door_caller);                   /* resume caller     */
}

/*──── callee-side accessors (zero cost when inlined) ─────────*/
const void *door_message(void) { return door_slab;        }
uint8_t     door_words  (void) { return door_msg_words;    }
uint8_t     door_flags  (void) { return door_msg_flags;    }
