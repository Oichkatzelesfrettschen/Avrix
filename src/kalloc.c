/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#include "kalloc.h"
#include <stddef.h>

#define HEAP_SIZE 256u

typedef struct block {
    struct block *next; /**< Next block in free list */
    uint8_t size;       /**< Size of the user allocation */
} block_t;

static uint8_t heap[HEAP_SIZE];
static uint8_t *heap_top;
static block_t *freelist;

void kalloc_init(void) {
    heap_top = heap;
    freelist = NULL;
}

void *kalloc(uint8_t size) {
    if (size == 0)
        return NULL;
    /* word align */
    if (size & 1)
        size++;

    block_t **prev = &freelist;
    for (block_t *b = freelist; b; b = b->next) {
        if (b->size >= size) {
            *prev = b->next;
            return (void *)(b + 1);
        }
        prev = &b->next;
    }

    if (heap_top + sizeof(block_t) + size > heap + HEAP_SIZE)
        return NULL;

    block_t *blk = (block_t *)heap_top;
    blk->size = size;
    heap_top += sizeof(block_t) + size;
    return (void *)(blk + 1);
}

void kfree(void *ptr) {
    if (!ptr)
        return;
    block_t *blk = (block_t *)ptr - 1;
    blk->next = freelist;
    freelist = blk;
}
