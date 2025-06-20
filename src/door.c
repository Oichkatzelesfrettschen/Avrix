#include "door.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* Shared message slab. Placed in .noinit so it survives soft resets. */
uint8_t door_slab[128] __attribute__((section(".noinit")));

/* Per-task door vector, defined by the scheduler. */
extern door_t door_vec[DOOR_SLOTS];

void door_call(uint8_t idx, const void *msg)
{
    door_slab[0] = (uint16_t)msg & 0xFF;
    door_slab[1] = (uint16_t)msg >> 8;
    (void)idx; /* placeholder for task switch */
}
