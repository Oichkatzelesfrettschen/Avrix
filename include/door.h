/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/*────────────────────────── door.h ────────────────────────────
   µ-UNIX — Descriptor-based RPC (“Doors”) for ATmega328P
   -------------------------------------------------------------
   • Solaris-style synchronous call / return
   • Flattened Cap’n-Proto slab (zero-copy)
   • 4 descriptors per task by default (configurable)

   Public API is pure C23 but wrapped in extern "C" for C++ users.
   -----------------------------------------------------------------*/
#ifndef AVR_DOOR_H
#define AVR_DOOR_H

#include <stdint.h>
#include "compat.h"            /* AVR_UNUSED, likely lives in include/ */
#include "nk_task.h"           /* NK_MAX_TASKS for door_vec */

#ifdef __cplusplus
extern "C" {
#endif

/*──────────────────────── 1. Configuration ─────────────────────*/
#ifndef DOOR_SLOTS
#  define DOOR_SLOTS       4       /* descriptors per task (0-15 allowed) */
#endif
#ifndef DOOR_SLAB_SIZE
#  define DOOR_SLAB_SIZE 128       /* shared slab bytes, must be /8       */
#endif

_Static_assert(DOOR_SLOTS <= 15,      "door slots fit in 4-bit fid");
_Static_assert(DOOR_SLAB_SIZE % 8 == 0, "slab must be 8-byte aligned");

/*──────────────────────── 2. Descriptor type ───────────────────*/
/**
 * @struct door_t
 * @brief Door descriptor stored in each task’s vector (in .noinit).
 *
 * | Field   | Bits | Purpose                              |
 * |---------|------|--------------------------------------|
 * | tgt_tid |  8   | Callee task-ID (0-7)                 |
 * | words   |  4   | Payload length in 8-byte words (1-15)|
 * | flags   |  4   | Protocol flags                       |
 */
typedef struct {
    uint8_t tgt_tid;                /**< Callee task-ID (0-7).           */
    uint8_t words : 4;              /**< Msg length in 8-byte words.     */
    uint8_t flags : 4;              /**< Option flags (bit-mapped).      */
} door_t;

/*──────────────────────── 3. Global slab ───────────────────────*/
/* One slab shared by *all* tasks. Lives in .noinit so reboot
   persistence works when desired.  Defined in door.c */
extern uint8_t door_slab[DOOR_SLAB_SIZE];
extern door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

/*──────────────────────── 4. User API ──────────────────────────*/
/**
 * Install / update a door descriptor.
 *
 * @param idx     index in per-task descriptor vector
 * @param target  callee task-id (0-7)
 * @param words   payload length (1-15) in 8-byte words
 * @param flags   user-defined flags (4 bits)
 */
void door_register(uint8_t idx, uint8_t target,
                   uint8_t words, uint8_t flags);

/**
 * Synchronous call: copies *words*×8 bytes from *buf* into slab,
 * context-switches to target, blocks caller until `door_return()`.
 *
 * @param idx  descriptor index
 * @param buf  pointer to source payload
 */
void door_call(uint8_t idx, const void *buf);

/**
 * Callee returns to the blocker (caller).  Must be executed in the
 * task that received the door call.
 */
void door_return(void);

/*── Callee-side accessors (valid only between call entry & return) ──*/
const void *door_message(void);      /* pointer inside slab            */
uint8_t     door_words(void);        /* length in 8-byte words         */
uint8_t     door_flags(void);        /* flags supplied by caller       */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* AVR_DOOR_H */
