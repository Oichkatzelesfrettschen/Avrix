#include "nk_spinlock.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    nk_spinlock_lock(&lock, 1u);
    assert(lock.dag_mask == 1u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    bool ok = nk_spinlock_trylock(&lock, 2u);
    assert(ok);
    nk_spinlock_unlock(&lock);

    nk_spinlock_lock_rt(&lock, 3u);
    assert(lock.rt_mode == 1u);
    nk_spinlock_unlock_rt(&lock);

    nk_spinlock_matrix_set(&lock, 0, 0xdeadbeef);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.matrix[0] == 0xdeadbeef);

    printf("dag=%u\n", snap.dag_mask);
    return 0;
}
