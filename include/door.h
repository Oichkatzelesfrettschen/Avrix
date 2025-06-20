#ifndef AVR_DOOR_H
#define AVR_DOOR_H

#include <stdint.h>
#include "compat.h"
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

/** Call a door by index with a pointer to the message payload. */
void door_call(uint8_t idx AVR_UNUSED, const void *msg);

#ifdef __cplusplus
}
#endif

#endif /* AVR_DOOR_H */
