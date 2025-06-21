#include "nk_spinlock.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int main(void)
{
    nk_superlock_t lock = NK_SUPERLOCK_STATIC_INIT;
    nk_superlock_init(&lock);

    nk_superlock_lock(&lock, 0x1u);
    nk_superlock_capnp_t snap;
    nk_superlock_encode(&lock, &snap);
    assert(snap.dag_mask == 0x1u);

    nk_superlock_unlock(&lock);
    assert(lock.dag_mask == 0u);
    assert(nk_bkl.base == 0);

    /* exercise trylock and decode helpers */
    bool ok = nk_superlock_trylock(&lock, 0x3u);
    assert(ok);
    nk_superlock_matrix_set(&lock, 2, 0xdeadbeef);
    nk_superlock_capnp_t snap2;
    nk_superlock_encode(&lock, &snap2);
    nk_superlock_unlock(&lock);
    nk_superlock_decode(&lock, &snap2);
    assert(lock.dag_mask == 0x3u);
    assert(lock.matrix[2] == 0xdeadbeef);
    nk_superlock_release(&lock);

    /* fine-grained real-time mode */
    ok = nk_superlock_trylock_rt(&lock, 0x5u);
    assert(ok && lock.rt_mode);
    nk_superlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u);

    nk_superlock_lock_rt(&lock, 0x2u);
    nk_superlock_unlock_rt(&lock);

    printf("superlock encoded mask=%u\n", snap.dag_mask);
    return 0;
}
