#include "door.h"
#include "task.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* Shared message slab. Placed in .noinit so it survives soft resets. */
uint8_t door_slab[DOOR_SLAB_SIZE] __attribute__((section(".noinit")));

/* Track the calling task and message size for reply handling. */
static uint8_t door_return_tid;
static uint8_t door_msg_len;

/* Per-task door vector, defined by the scheduler. */
extern door_t door_vec[DOOR_SLOTS];

void door_call(uint8_t idx, const void *msg)
{
    if (idx >= DOOR_SLOTS || msg == NULL) {
        return;
    }

    /* Look up descriptor and copy message into the shared slab. */
    door_t d = door_vec[idx];
    door_msg_len = d.words * 8;
    if (door_msg_len > DOOR_SLAB_SIZE) {
        door_msg_len = DOOR_SLAB_SIZE;
    }
    memcpy(door_slab, msg, door_msg_len);

    /* Save caller ID and switch to callee task. */
    door_return_tid = scheduler_current_tid();
    scheduler_switch_to(d.tgt_tid);

    /* Upon return, copy the reply back to the caller's buffer. */
    memcpy((void *)msg, door_slab, door_msg_len);
}

const void *door_handle(const void *reply)
{
    if (reply == NULL) {
        /* First phase: provide pointer to received message. */
        return door_slab;
    }

    /* Copy reply into the shared slab and resume caller. */
    memcpy(door_slab, reply, door_msg_len);
    scheduler_switch_to(door_return_tid);
    return NULL;
}
