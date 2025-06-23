#include "door.h"
#include "nk_task.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static jmp_buf ctx_client;
static jmp_buf ctx_server;
static uint8_t current_idx;

typedef struct {
    jmp_buf *target;
    uint8_t words;
    uint8_t flags;
} entry_t;

static entry_t table[DOOR_SLOTS];
uint8_t door_slab[DOOR_SLAB_SIZE];

void scheduler_init(void) {}
void scheduler_run(void) {}

void door_register(uint8_t idx, uint8_t target, uint8_t words, uint8_t flags)
{
    (void)target; /* single-server demo */
    table[idx].target = &ctx_server;
    table[idx].words  = words;
    table[idx].flags  = flags;
}

void door_call(uint8_t idx, const void *buf)
{
    memcpy(door_slab, buf, table[idx].words * 8);
    current_idx = idx;
    if (setjmp(ctx_client) == 0)
        longjmp(ctx_server, 1);
    memcpy((void *)buf, door_slab, table[idx].words * 8);
}

void door_return(void)
{
    longjmp(ctx_client, 1);
}

const void *door_message(void) { return door_slab; }
uint8_t door_words(void) { return table[current_idx].words; }
uint8_t door_flags(void) { return table[current_idx].flags; }

static void server_task(void)
{
    if (setjmp(ctx_server)) {
        const char *msg = (const char *)door_message();
        printf("server: %s\n", msg);
        const char reply[] = "pong";
        memcpy(door_slab, reply, sizeof reply);
        door_return();
    }
}

static void client_task(void)
{
    char buf[8] = "ping";
    door_call(0, buf);
    printf("client: %s\n", buf);
}

int main(void)
{
    door_register(0, 1, 1, 0); /* idx 0 → server task, one 8-byte word */
    server_task();            /* primed, waiting for door_call */
    client_task();
    return 0;
}

