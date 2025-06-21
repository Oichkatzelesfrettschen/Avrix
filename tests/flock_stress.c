#include "nk_lock.h"
#include <stdio.h>

int main(void) {
    nk_flock_t l;
    nk_flock_init(&l);
    for (int i = 0; i < 100000; ++i) {
        nk_flock_lock(&l);
        nk_flock_unlock(&l);
    }
    puts("flock stress completed");
    return 0;
}
