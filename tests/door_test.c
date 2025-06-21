#include <stdint.h>

void scheduler_init(void) {}
void scheduler_run(void) {}

#include "door.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

/*
 * Host-side unit test for the Door RPC primitives.
 * The real scheduler and assembly trampoline are replaced by
 * lightweight stubs so that door_call() and door_return()
 * can be exercised without an AVR context.
 */

/* -------------------------------------------------------------
 *  Minimal scheduler stubs
 * -----------------------------------------------------------*/
static uint8_t current_tid;
static void (*service_table[NK_MAX_TASKS])(void);

door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

uint8_t nk_cur_tid(void)
{
    return current_tid;
}

void nk_switch_to(uint8_t tid)
{
    /* context switch is a no-op in the host stub */
    current_tid = tid;
}

/* _nk_door replicates the AVR trampoline.  It copies the request
 * payload into the shared slab and invokes the callee handler.
 */
void _nk_door(const void *src, uint8_t len, uint8_t tid)
{
    memcpy(door_slab, src, len);
    current_tid = tid;
    if (service_table[tid])
        service_table[tid]();
    /* door_return() will restore current_tid to the caller */
}

#include "../src/door.c"

/* -------------------------------------------------------------
 *  Door services for the test
 * -----------------------------------------------------------*/
static void door_echo(void)
{
    assert(door_words() == 1);
    assert(door_flags() == 0);

    memcpy(door_slab, door_message(), 8);
    door_return();
}

static void door_crc_srv(void)
{
    assert(door_words() == 1);
    assert(door_flags() == 1);

    const uint8_t *msg = (const uint8_t *)door_message();
    uint8_t crc = msg[8];
    assert(crc == crc8_maxim(msg, 8));

    const char reply[8] = "crc_ok\0"; /* 8 bytes including NUL */
    memcpy(door_slab, reply, 8);
    door_return();
}

/* -------------------------------------------------------------
 *  Test driver
 * -----------------------------------------------------------*/
int main(void)
{
    /* Descriptor without CRC */
    current_tid = 0;
    service_table[1] = door_echo;
    door_register(0, 1, 1, 0);

    char buf[8] = "payload";
    door_call(0, buf);
    assert(memcmp(buf, "payload", 8) == 0);

    /* Descriptor with CRC */
    service_table[1] = door_crc_srv;
    door_register(1, 1, 1, 1);

    char buf2[8] = "checkme";
    door_call(1, buf2);
    assert(memcmp(buf2, "crc_ok\0", 8) == 0);

    puts("door rpc ok");
    return 0;
}

