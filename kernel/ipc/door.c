/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file door.c
 * @brief Portable Door RPC Implementation
 *
 * Zero-copy RPC for embedded systems. Originally designed for ATmega328P,
 * now portable via HAL abstraction.
 *
 * ## Memory Footprint
 * - Flash: ~700 bytes (with CRC-8)
 * - SRAM: DOOR_SLAB_SIZE + (NK_MAX_TASKS * DOOR_SLOTS * 2) + 3 bytes
 * - Example: 128 + (8 * 4 * 2) + 3 = 195 bytes for 8 tasks
 *
 * ## Thread Safety
 * - Not reentrant: only one door call per task at a time
 * - Uses HAL atomics and memory barriers for cross-task communication
 * - Interrupts disabled during context switch
 */

#include "door.h"
#include "arch/common/hal.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * COMPILE-TIME VALIDATION
 *═══════════════════════════════════════════════════════════════════*/

_Static_assert(DOOR_SLAB_SIZE % 8 == 0, "slab must be multiple of 8 bytes");
_Static_assert(DOOR_SLOTS <= 15, "descriptor index fits in 4 bits");
_Static_assert(sizeof(door_t) == 2, "door_t must remain 2 bytes");

/*═══════════════════════════════════════════════════════════════════
 * PERSISTENT STATE (.noinit section)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Shared message buffer
 *
 * Lives in .noinit section for warm-reboot persistence.
 * All door messages pass through this single buffer.
 */
HAL_SECTION(".noinit")
uint8_t door_slab[DOOR_SLAB_SIZE];

/**
 * @brief Per-task door descriptor vectors
 *
 * door_vec[tid][idx] holds the descriptor for task tid, slot idx.
 */
HAL_SECTION(".noinit")
door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

/**
 * @brief Calling task ID (saved during door_call)
 */
static volatile uint8_t door_caller HAL_SECTION(".noinit");

/**
 * @brief Message length in 8-byte words (1-15)
 */
static volatile uint8_t door_words_v;

/**
 * @brief Protocol flags (4 bits)
 */
static volatile uint8_t door_flags_v;

/*═══════════════════════════════════════════════════════════════════
 * CRC-8 (Dallas/Maxim polynomial: 0x31)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief CRC-8 lookup table (Dallas/Maxim polynomial)
 *
 * Polynomial: x^8 + x^5 + x^4 + 1 (0x31)
 * Used for optional message validation (flag bit 0).
 */
static const uint8_t crc8_lut[32] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D
};

/**
 * @brief Compute CRC-8 (Dallas/Maxim)
 *
 * @param p Pointer to data
 * @param len Length in bytes
 * @return CRC-8 value
 */
static uint8_t crc8_maxim(const uint8_t *p, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        uint8_t in = *p++ ^ crc;
        crc = (crc << 5)
            ^ crc8_lut[in >> 3]
            ^ (in & 0x07);
    }
    return crc;
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - DOOR MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Register a door descriptor
 *
 * Creates or updates a door descriptor in the calling task's vector.
 *
 * @param idx Door descriptor index (0 to DOOR_SLOTS-1)
 * @param target Target task ID (callee)
 * @param words Message length in 8-byte words (1-15)
 * @param flags Protocol flags (bit 0: enable CRC-8)
 */
void door_register(uint8_t idx, uint8_t target,
                   uint8_t words, uint8_t flags) {
    /* Validate arguments */
    if (idx >= DOOR_SLOTS) {
        return;  /* Invalid slot index */
    }
    if (words == 0 || (uint16_t)words * 8 > DOOR_SLAB_SIZE) {
        return;  /* Invalid message size */
    }

    /* Get current task ID */
    const uint8_t tid = nk_current_tid();

    /* Install descriptor */
    door_vec[tid][idx] = (door_t){
        .tgt_tid = (uint8_t)(target & (NK_MAX_TASKS - 1)),
        .words   = (uint8_t)(words  & 0x0F),
        .flags   = (uint8_t)(flags  & 0x0F)
    };

    /* Memory barrier to ensure descriptor is visible */
    hal_memory_barrier();
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - DOOR COMMUNICATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Call a door (synchronous RPC)
 *
 * 1. Copies message from `buf` to shared slab
 * 2. Optionally appends CRC-8 (if flag bit 0 set)
 * 3. Context-switches to target task
 * 4. Blocks until target calls door_return()
 * 5. Copies reply from slab back to `buf`
 *
 * @param idx Door descriptor index
 * @param buf Pointer to message buffer (in/out)
 */
void door_call(uint8_t idx, const void *buf) {
    /* Get current task ID */
    const uint8_t caller = nk_current_tid();

    /* Validate slot index */
    if (idx >= DOOR_SLOTS) {
        return;
    }

    /* Load descriptor */
    const door_t d = door_vec[caller][idx];
    if (d.words == 0) {
        return;  /* Descriptor empty */
    }

    /* Calculate message size */
    const uint8_t nbytes = (uint8_t)(d.words * 8);
    const void   *src    = buf;
    uint8_t       len    = nbytes;

    /* If CRC flag is set, copy message and append CRC */
    if (d.flags & 0x01) {
        memcpy(door_slab, buf, nbytes);
        door_slab[nbytes] = crc8_maxim(door_slab, nbytes);
        src = door_slab;
        ++len;
    } else {
        /* No CRC - copy directly to slab */
        memcpy(door_slab, src, len);
    }

    /* Save call context */
    door_caller  = caller;
    door_words_v = d.words;
    door_flags_v = d.flags;

    /* Memory barrier before context switch */
    hal_memory_barrier();

    /* Context switch to callee (blocks until door_return) */
    nk_switch_to(d.tgt_tid);

    /* Callee has returned - copy reply back to caller's buffer */
    hal_memory_barrier();
    memcpy((void *)buf, door_slab, nbytes);
}

/**
 * @brief Return from a door call
 *
 * Must be called by the callee (target task) to resume the caller.
 * The caller's buffer is updated with the contents of door_slab.
 */
void door_return(void) {
    /* Memory barrier before context switch */
    hal_memory_barrier();

    /* Resume caller task */
    uint8_t caller_tid = door_caller;
    nk_switch_to(caller_tid);
}

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - CALLEE HELPERS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Get pointer to incoming message
 *
 * @return Pointer to message in shared slab
 */
const void *door_message(void) {
    return door_slab;
}

/**
 * @brief Get message length in 8-byte words
 *
 * @return Message length (1-15 words = 8-120 bytes)
 */
uint8_t door_words(void) {
    return door_words_v;
}

/**
 * @brief Get door flags
 *
 * @return 4-bit flags field from descriptor
 */
uint8_t door_flags(void) {
    return door_flags_v;
}
