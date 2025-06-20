#include "spinlock.h"
#include <avr/io.h>

/* Basic spinlock using IO registers for minimal latency. */

void spinlock_acquire(uint8_t lock_addr, uint8_t mark)
{
    volatile uint8_t *reg = (volatile uint8_t *)lock_addr;
    while (1) {
        if (*reg == 0) {
            *reg = mark;
            if (*reg == mark)
                return; /* acquired */
        } else if (*reg == mark) {
            return; /* recursive acquisition */
        }
    }
}

void spinlock_release(uint8_t lock_addr)
{
    volatile uint8_t *reg = (volatile uint8_t *)lock_addr;
    *reg = 0;
}

