#include "kalloc.h"
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

int main(void)
{
    kalloc_init();

    /* Allocate three blocks of varying size. */
    void *a = kalloc(10);
    void *b = kalloc(20);
    void *c = kalloc(30);
    assert(a && b && c);

    /* Free in non-LIFO order to build up the free list. */
    kfree(b);
    kfree(a);
    kfree(c);

    /* Most recently freed block should be reused first. */
    void *d = kalloc(5);
    assert(d == c);

    /* Remaining blocks in the free list should satisfy requests */
    void *e = kalloc(10);
    assert(e == a);
    void *f = kalloc(20);
    assert(f == b);

    /* Fill the heap until exhaustion. */
    void *blocks[64];
    size_t count = 0;
    for (; count < 64; ++count) {
        blocks[count] = kalloc(32);
        if (!blocks[count])
            break;
    }

    /* Next allocation must fail when the heap is exhausted. */
    void *fail = kalloc(32);
    assert(fail == NULL);

    /* Free all allocated blocks and ensure space is reusable. */
    for (size_t i = 0; i < count; ++i)
        kfree(blocks[i]);

    void *g = kalloc(32);
    assert(g != NULL);

    printf("kalloc blocks:%zu\n", count);
    return 0;
}
