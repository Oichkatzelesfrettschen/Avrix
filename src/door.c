#include "door.h"
#include "task.h"
#include <stdint.h>

/* Shared message slab. Placed in .noinit so it survives soft resets. */
uint8_t door_slab[128] __attribute__((section(".noinit")));

/*
 * Per-task door descriptor table. Defined in task.c and indexed by
 * [task_id][slot].
 */
extern door_t door_vec[MAX_TASKS][DOOR_SLOTS];

/* Task id of the caller while a door is active. */
static uint8_t door_caller;

void door_call(uint8_t idx, const void *msg)
{
    uint8_t caller = task_current_id();
    if (idx >= DOOR_SLOTS) {
        return;
    }

    door_t d = door_vec[caller][idx];
    door_slab[0] = (uint16_t)msg & 0xFF;
    door_slab[1] = (uint16_t)msg >> 8;
    door_slab[2] = d.words;
    door_slab[3] = d.flags;

    door_caller = caller;
    task_switch_to(d.tgt_tid);
}

void door_register(uint8_t idx, uint8_t target, uint8_t words, uint8_t flags)
{
    uint8_t tid = task_current_id();
    if (idx >= DOOR_SLOTS) {
        return;
    }

    door_vec[tid][idx].tgt_tid = target;
    door_vec[tid][idx].words   = words & 0x0F;
    door_vec[tid][idx].flags   = flags & 0x0F;
}

void door_return(void)
{
    task_switch_to(door_caller);
}

const void *door_message(void)
{
    uint16_t ptr = door_slab[0] | ((uint16_t)door_slab[1] << 8);
    return (const void *)(uintptr_t)ptr;
}

uint8_t door_words(void)
{
    return door_slab[2];
}

uint8_t door_flags(void)
{
    return door_slab[3];
}
