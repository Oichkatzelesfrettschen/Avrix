#include "nk_spinlock.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Self-test for the unified nk_spinlock API.
 *
 * Exercises:
 *  - blocking lock/unlock
 *  - trylock/unlock
 *  - encode/decode round-trip
 *  - alias acquire/release
 *  - real-time (bypass) lock variants
 */
int main(void)
{
    /* 1. Initialize spinlock */
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    /* 2. Blocking lock/unlock */
    nk_spinlock_lock(&lock, 0x1u);
    assert(lock.dag_mask == 0x1u);
    nk_spinlock_capnp_t snap1;
    nk_spinlock_encode(&lock, &snap1);
    assert(snap1.dag_mask == lock.dag_mask);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* 3. Trylock + unlock */
    bool ok = nk_spinlock_trylock(&lock, 0x2u);
    assert(ok && lock.dag_mask == 0x2u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* 4. Trylock + encode/decode round-trip */
    ok = nk_spinlock_trylock(&lock, 0x3u);
    assert(ok && lock.dag_mask == 0x3u);
    nk_spinlock_matrix_set(&lock, 2, 0xDEADBEEFu);
    nk_spinlock_capnp_t snap2;
    nk_spinlock_encode(&lock, &snap2);
    nk_spinlock_unlock(&lock);

    /* Restore state from snapshot */
    nk_spinlock_decode(&lock, &snap2);
    assert(lock.dag_mask   == 0x3u);
    assert(lock.matrix[2]  == 0xDEADBEEF);

    /* 5. Alias acquire/release */
    nk_spinlock_acquire(&lock, 0x4u);
    assert(lock.dag_mask == 0x4u);
    nk_spinlock_release(&lock);
    assert(lock.dag_mask == 0u);

    /* 6. Real-time blocking lock/unlock */
    nk_spinlock_lock_rt(&lock, 0x5u);
    assert(lock.rt_mode == 1u && lock.dag_mask == 0x5u);
    nk_spinlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u);

    /* 7. Real-time trylock/unlock */
    ok = nk_spinlock_trylock_rt(&lock, 0x6u);
    assert(ok && lock.rt_mode == 1u && lock.dag_mask == 0x6u);
    nk_spinlock_release_rt(&lock);
    assert(lock.rt_mode == 0u);

    printf("✅ All nk_spinlock self-tests passed.\n");
    return 0;
}
