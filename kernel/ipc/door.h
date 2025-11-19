/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file door.h
 * @brief Portable Door RPC System
 *
 * Implements Solaris-style synchronous RPC ("Doors") for embedded systems.
 * Provides zero-copy message passing with descriptor-based routing.
 *
 * Originally designed for ATmega328P, now portable across all architectures
 * via HAL abstraction.
 *
 * ## Features
 * - Synchronous call/return semantics
 * - Zero-copy via shared slab buffer
 * - Per-task descriptor vectors (configurable slots)
 * - Optional CRC-8 validation (Dallas/Maxim polynomial)
 * - Persistent state across reboots (via .noinit section)
 *
 * ## Usage
 * ```c
 * // Server task (tid=1)
 * void server_task(void) {
 *     while (1) {
 *         // Wait for door call...
 *         const void *msg = door_message();
 *         uint8_t len = door_words() * 8;
 *         // Process request, write reply to door_slab
 *         door_return();  // Return to caller
 *     }
 * }
 *
 * // Client task (tid=0)
 * void client_task(void) {
 *     door_register(0, 1, 2, 0);  // idx=0, target=1, 16 bytes, no flags
 *     uint8_t req[16] = { ... };
 *     door_call(0, req);           // Blocks until server returns
 *     // req[] now contains reply
 * }
 * ```
 */

#ifndef KERNEL_IPC_DOOR_H
#define KERNEL_IPC_DOOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "kernel/sched/scheduler.h"  /* nk_current_tid, task IDs */

/*═══════════════════════════════════════════════════════════════════
 * CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Number of door descriptors per task
 *
 * Each task can have up to DOOR_SLOTS registered door endpoints.
 * Valid range: 1-15 (4-bit field in descriptor).
 */
#ifndef DOOR_SLOTS
#  define DOOR_SLOTS 4
#endif

/**
 * @brief Shared slab buffer size in bytes
 *
 * All door messages use this single shared buffer. Must be a multiple
 * of 8 for alignment.
 */
#ifndef DOOR_SLAB_SIZE
#  define DOOR_SLAB_SIZE 128
#endif

/* Compile-time validation */
_Static_assert(DOOR_SLOTS <= 15, "door slots must fit in 4-bit field");
_Static_assert(DOOR_SLAB_SIZE % 8 == 0, "slab must be 8-byte aligned");

/*═══════════════════════════════════════════════════════════════════
 * DOOR DESCRIPTOR
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Door descriptor (2 bytes)
 *
 * Stored in each task's descriptor vector. Defines a door endpoint.
 *
 * | Field   | Bits | Purpose                              |
 * |---------|------|--------------------------------------|
 * | tgt_tid |  8   | Target task ID (callee)              |
 * | words   |  4   | Message length in 8-byte words (1-15)|
 * | flags   |  4   | Protocol flags (bit 0 = CRC-8)       |
 */
typedef struct {
    uint8_t tgt_tid;        /**< Target task ID */
    uint8_t words : 4;      /**< Message length (8-byte words) */
    uint8_t flags : 4;      /**< Flags (bit 0 = CRC-8) */
} door_t;

_Static_assert(sizeof(door_t) == 2, "door_t must be 2 bytes");

/*═══════════════════════════════════════════════════════════════════
 * GLOBAL STATE
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Shared message buffer
 *
 * All door calls copy messages into this buffer. Lives in .noinit
 * section for persistence across warm reboots.
 */
extern uint8_t door_slab[DOOR_SLAB_SIZE];

/**
 * @brief Per-task door descriptor vectors
 *
 * door_vec[tid][idx] holds the door descriptor for task `tid`,
 * descriptor index `idx`.
 */
extern door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

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
 *
 * @note If idx >= DOOR_SLOTS or words == 0, the call is ignored.
 * @note Maximum message size is min(words*8, DOOR_SLAB_SIZE).
 */
void door_register(uint8_t idx, uint8_t target,
                   uint8_t words, uint8_t flags);

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
 *
 * @note `buf` must be at least (words*8) bytes.
 * @note The buffer is modified in-place with the reply.
 * @note Does nothing if idx >= DOOR_SLOTS or descriptor is empty.
 *
 * @warning Not reentrant! Only one door call can be active at a time
 *          per task.
 */
void door_call(uint8_t idx, const void *buf);

/**
 * @brief Return from a door call
 *
 * Must be called by the callee (target task) to resume the caller.
 * The caller's buffer is updated with the contents of door_slab.
 *
 * @note Must be called from the task that received the door call.
 * @note After return, caller resumes execution after door_call().
 */
void door_return(void);

/*═══════════════════════════════════════════════════════════════════
 * PUBLIC API - CALLEE HELPERS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Get pointer to incoming message
 *
 * Valid only between door call entry and door_return().
 *
 * @return Pointer to message in shared slab
 */
const void *door_message(void);

/**
 * @brief Get message length in 8-byte words
 *
 * Valid only between door call entry and door_return().
 *
 * @return Message length (1-15 words = 8-120 bytes)
 */
uint8_t door_words(void);

/**
 * @brief Get door flags
 *
 * Valid only between door call entry and door_return().
 *
 * @return 4-bit flags field from descriptor
 */
uint8_t door_flags(void);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_IPC_DOOR_H */
