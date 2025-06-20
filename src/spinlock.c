#include "spinlock.h"

/**
 * Acquire a quaternion spinlock using IO instructions. The lock register must
 * be within the lower IO space so that `in` and `out` can be utilised. The
 * function spins until the caller owns the lock. It returns only after the lock
 * state equals `mark`.
 */
void spinlock_acquire(uint8_t lock_addr, uint8_t mark) {
    __asm__ __volatile__(
        "1: in __tmp_reg__, %[lock]\n"    // load lock byte
        "   tst __tmp_reg__\n"          // test for zero
        "   brne 2f\n"                 // branch if not zero
        "   out %[lock], %[mark]\n"     // attempt to acquire
        "   in __tmp_reg__, %[lock]\n" // verify
        "   cp __tmp_reg__, %[mark]\n"  // did we get it?
        "   brne 1b\n"                 // retry
        "   rjmp 3f\n"                 // success
        "2: cp __tmp_reg__, %[mark]\n" // recursive acquisition?
        "   breq 3f\n"                 // already ours
        "   rjmp 1b\n"                 // spin
        "3:\n"
        :
        : [lock] "I" (lock_addr), [mark] "r" (mark)
        : "__tmp_reg__"
    );
}

/**
 * Release the spinlock by clearing the lock register.
 */
void spinlock_release(uint8_t lock_addr) {
    __asm__ __volatile__("out %[lock], __zero_reg__" :: [lock] "I" (lock_addr));
}
