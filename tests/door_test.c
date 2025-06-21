#include <stdint.h>
/* Stubs required for door.c aliases ---------------------------------- */
void scheduler_init(void) {}
void scheduler_run(void) {}

#include "door.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ───── Stub scheduler ------------------------------------------------ */
static uint8_t g_cur_tid;                 /* simulated task ID */

uint8_t nk_cur_tid(void) { return g_cur_tid; }

void nk_switch_to(uint8_t tid) { g_cur_tid = tid; }

/* Door descriptor table (normally in scheduler) */
door_t door_vec[NK_MAX_TASKS][DOOR_SLOTS];

/* Forward declaration of the callee service. */
static void door_service(void);

/* Pull in the implementation under test. */
#include "../src/door.c"

/* ───── Minimal _nk_door stub ---------------------------------------- */
void _nk_door(const void *src, uint8_t len, uint8_t tid)
{
    memcpy(door_slab, src, len);  /* copy request into slab */
    nk_switch_to(tid);            /* enter callee context  */
    door_service();               /* handle call           */
}

/* ───── Callee implementation --------------------------------------- */
static void door_service(void)
{
    assert(nk_cur_tid() == 1);            /* switched to callee */
    assert(door_words() == 1);            /* one 8-byte word    */
    assert(door_flags() == 0);            /* no flags enabled   */

    const uint8_t *msg = door_message();
    assert(msg[0] == 42);                 /* validate payload   */

    door_slab[0] = (uint8_t)(msg[0] + 1); /* craft reply        */
    door_return();                        /* resume caller      */
}

/* ───── Test cases ---------------------------------------------------- */
int main(void)
{
    memset(door_vec, 0, sizeof door_vec);
    g_cur_tid = 0;                        /* caller task */

    /* 1. invalid registrations must be ignored */
    door_register(DOOR_SLOTS, 1, 1, 0);   /* bad index */
    for (unsigned i = 0; i < DOOR_SLOTS; ++i)
        assert(door_vec[0][i].words == 0);

    door_register(0, 1, 0, 0);            /* zero words */
    assert(door_vec[0][0].words == 0);

    door_register(0, 1, (DOOR_SLAB_SIZE / 8) + 1, 0); /* overflow */
    assert(door_vec[0][0].words == 0);

    /* 2. install valid descriptor */
    door_register(0, 1, 1, 0);
    assert(door_vec[0][0].tgt_tid == 1);
    assert(door_vec[0][0].words   == 1);
    assert(door_vec[0][0].flags   == 0);

    /* 3. round-trip message */
    uint8_t buf[8] = { 42 };
    door_call(0, buf);
    assert(buf[0] == 43);                 /* callee echoed +1   */
    assert(g_cur_tid == 0);               /* returned to caller */

    puts("door RPC passed");
    return 0;
}

