/*──────────────────────────── door.c ─────────────────────────────
   µ-UNIX “Doors” — zero-copy RPC for the ATmega328P nanokernel
   -------------------------------------------------------------
   • < 700 B flash, 3 B SRAM (gcc-avr 14 -Oz -flto)
   • One 128-byte slab, 4 descriptors per task
   • Optional Dallas/Maxim CRC-8 on request payload (flag bit 0)
   • Entire module is freestanding, pure C23 and host-build friendly
   ----------------------------------------------------------------*/

#include "door.h"
#include "nk_task.h"          /* nk_cur_tid · nk_switch_to */
#include <string.h>

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#else                          /* host-side unit tests */
  #define PROGMEM
  #define pgm_read_byte(p) (*(const uint8_t *)(p))
#endif

/*───────────────── 0. Compile-time sanity checks ───────────────────*/
_Static_assert(DOOR_SLAB_SIZE % 8 == 0,  "slab must be multiple of 8 bytes");
_Static_assert(DOOR_SLOTS      <= 15,   "descriptor index fits in 4 bits");
_Static_assert(sizeof(door_t)  == 2,    "door_t must remain 2 bytes");

/*───────────────── 1. Persistent state  (.noinit)  ─────────────────*/
uint8_t door_slab[DOOR_SLAB_SIZE]
        __attribute__((section(".noinit")));               /* shared payload */

static door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS]
        __attribute__((section(".noinit")));               /* per-task table */

static volatile uint8_t door_caller  __attribute__((section(".noinit")));
static volatile uint8_t door_words_v;   /* 1-15 words  (8-byte each) */
static volatile uint8_t door_flags_v;   /* low 4 bits */

/*───────────────── 2. CRC-8 (Dallas/Maxim, 0x31 poly) ──────────────*/
static uint8_t crc8_maxim(const uint8_t *p, uint8_t len)
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
        crc = (crc << 5)
            ^ pgm_read_byte(&lut[in >> 3])
            ^ (in & 0x07);
    }
    return crc;
}

/*───────────────── 3. Public API  ──────────────────────────────────*/
void door_register(uint8_t idx, uint8_t target,
                   uint8_t words, uint8_t flags)
{
    if (idx >= DOOR_SLOTS                 ||
        words == 0                        ||
        (uint16_t)words * 8 > DOOR_SLAB_SIZE)
        return;                                   /* invalid args */

    door_vec[nk_cur_tid()][idx] = (door_t){
        .tgt_tid = (uint8_t)(target & (NK_MAX_TASKS - 1)),
        .words   = (uint8_t)(words  & 0x0F),
        .flags   = (uint8_t)(flags  & 0x0F)
    };
}

void door_call(uint8_t idx, void *buf)        /* caller side */
{
    const uint8_t caller = nk_cur_tid();
    if (idx >= DOOR_SLOTS) return;

    const door_t d = door_vec[caller][idx];
    if (d.words == 0) return;                 /* descriptor empty */

    const uint8_t nbytes = (uint8_t)(d.words * 8);
    memcpy(door_slab, buf, nbytes);           /* → request slab */

    if (d.flags & 0x01)                       /* CRC flag bit 0 */
        door_slab[nbytes] = crc8_maxim(door_slab, nbytes);

    door_caller  = caller;
    door_words_v = d.words;
    door_flags_v = d.flags;

    nk_switch_to(d.tgt_tid);                  /* ↔ callee */

    memcpy(buf, door_slab, nbytes);           /* ← reply */
}

void door_return(void)                        /* callee side */
{
    nk_switch_to(door_caller);                /* resume caller */
}

/*────────── Callee helpers (inlineable, zero cost) ──────────*/
const void *door_message(void) { return door_slab;   }
uint8_t     door_words  (void) { return door_words_v; }
uint8_t     door_flags  (void) { return door_flags_v; }
