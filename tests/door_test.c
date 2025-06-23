/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 *
 * Host-side unit test for Door RPC primitives.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "door.h"

/*─── Stub scheduler & dispatch ─────────────────────────────────────────*/
static uint8_t current_tid;
uint8_t nk_cur_tid(void)        { return current_tid; }
void    nk_switch_to(uint8_t t) { current_tid = t;  }

static void (*service_table[NK_MAX_TASKS])(void);

/*─── Door storage ------------------------------------------------------*/
door_t  door_vec[NK_MAX_TASKS][DOOR_SLOTS];
extern uint8_t door_slab[DOOR_SLAB_SIZE];

/*─── Trampoline stub replicating AVR _nk_door ──────────────────────────*/
void _nk_door(const void *src, uint8_t len, uint8_t tid)
{
    memcpy(door_slab, src, len);
    current_tid = tid;
    if (service_table[tid])
        service_table[tid]();
    /* door_return() will switch back */
}

/*─── Pull in implementation under test ─────────────────────────────────*/
#include "../src/door.c"

/*─── Echo service ─────────────────────────────────────────────────────*/
static void door_echo(void)
{
    assert(current_tid == 1);
    assert(door_words() == 1);
    assert(door_flags() == 0);

    memcpy(door_slab, door_message(), 8);
    door_return();
}

/*─── CRC service ──────────────────────────────────────────────────────*/
static void door_crc_srv(void)
{
    assert(current_tid == 1);
    assert(door_words() == 1);
    assert(door_flags() == 1);

    const uint8_t *msg = (const uint8_t *)door_message();
    uint8_t crc = crc8_maxim(msg, 8);
    assert(crc == msg[8]);

    memcpy(door_slab, "crc_ok\0", 8);
    door_return();
}

/*─── Test driver ──────────────────────────────────────────────────────*/
int main(void)
{
    /* Init stub state */
    memset(door_vec, 0, sizeof door_vec);
    current_tid = 0;

    /* Test without CRC */
    service_table[1] = door_echo;
    door_register(0, 1, 1, 0);
    char buf1[8] = "payload";
    door_call(0, buf1);
    assert(memcmp(buf1, "payload", 8) == 0);

    /* Test with CRC */
    service_table[1] = door_crc_srv;
    door_register(1, 1, 1, 1);
    char buf2[8] = "checkme";
    door_call(1, buf2);
    assert(memcmp(buf2, "crc_ok\0", 8) == 0);

    puts("door RPC OK");
    return 0;
}
