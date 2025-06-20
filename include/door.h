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

/** Size in bytes of the shared message slab. */
#define DOOR_SLAB_SIZE 128

/** Shared message slab (defined in door.c). */
extern uint8_t door_slab[DOOR_SLAB_SIZE];

/**
 * @brief Call a door by index with a pointer to the message payload.
 *
 * The request is copied into a shared slab before switching to the
 * callee task. Upon return the reply is written back into the caller's
 * buffer.
 */
void door_call(uint8_t idx, const void *msg);

/**
 * @brief Handle a door request and optionally send the reply.
 *
 * Pass \c NULL to obtain a pointer to the received message. After
 * processing the request invoke it again with a pointer to the reply
 * buffer to resume the caller.
 */
const void *door_handle(const void *reply);

#ifdef __cplusplus
}
#endif

#endif /* AVR_DOOR_H */
