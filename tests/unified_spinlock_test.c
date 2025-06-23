#include "nk_superlock.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int main(void)
{
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    nk_spinlock_lock(&lock);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.dag_mask == 0x1u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    bool ok = nk_spinlock_trylock(&lock);
    assert(ok);
    nk_spinlock_matrix_set(&lock, 2, 0xdeadbeef);
    nk_spinlock_capnp_t snap2;
    nk_spinlock_encode(&lock, &snap2);
    nk_spinlock_unlock(&lock);
    nk_spinlock_decode(&lock, &snap2);
    assert(lock.matrix[2] == 0xdeadbeef);

    nk_spinlock_lock_rt(&lock);
    nk_spinlock_unlock_rt(&lock);

    printf("unified spin encoded mask=%u\n", snap.dag_mask);
    return 0;
}
