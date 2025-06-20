#ifndef AVR_DOOR_H
#define AVR_DOOR_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file door.h
 * @brief Tiny descriptor-based RPC interface inspired by Solaris Doors.
 */

/** Door descriptor holding target task and message size. */
typedef struct {
    uint8_t tgt_tid;  /**< Callee task id. */
    uint8_t words:4;  /**< Message length in 8-byte words. */
    uint8_t flags:4;  /**< Option flags for call semantics. */
} door_t;

/** Maximum number of doors per task. */
#define DOOR_SLOTS 4

/** Shared message slab (defined in door.c). */
extern uint8_t door_slab[128];

/**
 * @brief Invoke a registered door.
 *
 * The call stores \p msg in the shared slab, switches to the target
 * task and blocks until that task calls ::door_return().
 */
void door_call(uint8_t idx, const void *msg);

/**
 * @brief Register a door descriptor for the current task.
 *
 * The descriptor specifies the target task, message size (in 8-byte words)
 * and optional flags. It is stored in the per-task \c door_vec array managed
 * by the scheduler. Indices beyond \c DOOR_SLOTS are ignored.
 *
 * @param idx    Slot index in the door vector.
 * @param target Callee task identifier.
 * @param words  Message length measured in 8-byte words.
 * @param flags  Implementation-defined option flags.
 */
void door_register(uint8_t idx, uint8_t target, uint8_t words, uint8_t flags);

/**
 * @brief Return from a door invocation to the caller.
 */
void door_return(void);

/** Obtain the message pointer passed via ::door_call. */
const void *door_message(void);

/** Return the size of the message in 8-byte words. */
uint8_t door_words(void);

/** Retrieve option flags associated with the message. */
uint8_t door_flags(void);

#ifdef __cplusplus
}
#endif

#endif /* AVR_DOOR_H */
