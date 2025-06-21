#include "nk_spinlock.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int main(void)
{
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    nk_spinlock_lock(&lock, 0x1u);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.dag_mask == 0x1u);

    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);
    assert(nk_bkl.base == 0);

    /* exercise trylock and decode helpers */
    bool ok = nk_spinlock_trylock(&lock, 0x3u);
    assert(ok);
    nk_spinlock_matrix_set(&lock, 2, 0xdeadbeef);
    nk_spinlock_capnp_t snap2;
    nk_spinlock_encode(&lock, &snap2);
    nk_spinlock_unlock(&lock);
    nk_spinlock_decode(&lock, &snap2);
    assert(lock.dag_mask == 0x3u);
    assert(lock.matrix[2] == 0xdeadbeef);
    nk_spinlock_release(&lock);

    /* fine-grained real-time mode */
    ok = nk_spinlock_trylock_rt(&lock, 0x5u);
    assert(ok && lock.rt_mode);
    nk_spinlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u);

    nk_spinlock_lock_rt(&lock, 0x2u);
    nk_spinlock_unlock_rt(&lock);

    printf("spinlock encoded mask=%u\n", snap.dag_mask);
    return 0;
}
