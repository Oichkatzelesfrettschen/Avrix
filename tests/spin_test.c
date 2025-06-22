#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "nk_spinlock.h"
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#ifdef __x86_64__
#include <x86intrin.h>
#endif

/*─────────────────── 1 kHz timer interrupt ────────────────────────────*/
static volatile unsigned long tick_count = 0;

static void on_tick(int signum)
{
    (void)signum;
    ++tick_count;
}

/*───────────────────── High-resolution timer ──────────────────────────*/
static inline uint64_t rdcycles(void)
{
#if defined(__i386__) || defined(__x86_64__)
    return __rdtsc();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
#endif
}

int main(void)
{
    /*──────────────────── Install 1 kHz timer handler ──────────────────*/
    struct sigaction sa = { .sa_handler = on_tick };
    sigaction(SIGALRM, &sa, NULL);
    struct itimerval tv = { {0, 1000}, {0, 1000} };
    setitimer(ITIMER_REAL, &tv, NULL);

    /*────────────────── Initialize unified spinlock ─────────────────────*/
    nk_spinlock_global_init(); /* ensure nk_bkl is ready */
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    /*────────────────── Functional self-checks ─────────────────────────*/
    /* Blocking lock/unlock */
    nk_spinlock_lock(&lock, 1u);
    assert(lock.dag_mask == 1u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* Trylock/unlock */
    bool ok = nk_spinlock_trylock(&lock, 2u);
    assert(ok && lock.dag_mask == 2u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* Real-time blocking lock/unlock */
    nk_spinlock_lock_rt(&lock, 3u);
    assert(lock.rt_mode == 1u && lock.dag_mask == 3u);
    nk_spinlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u);

    /* Snapshot matrix encode/decode */
    nk_spinlock_matrix_set(&lock, 0, 0xDEADBEEFu);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.matrix[0] == 0xDEADBEEF);

    /*───────────────────────── Benchmark loop ──────────────────────────*/
    const unsigned loops = 2_000_000u;
    void *begin_brk = sbrk(0);
    uint64_t worst = 0;

    for (unsigned i = 0; i < loops; ++i) {
        uint64_t t0 = rdcycles();
        /* measure real-time spinlock path */
        nk_spinlock_lock_rt(&lock, 0u);
        nk_spinlock_unlock_rt(&lock);
        uint64_t dt = rdcycles() - t0;
        if (dt > worst) {
            worst = dt;
        }
    }

    void *end_brk = sbrk(0);
    /* Ensure no heap growth during locking */
    assert(begin_brk == end_brk);

    /*────────────────────────── Results ────────────────────────────────*/
    printf("ticks: %lu\nworst cycles: %" PRIu64 "\n", tick_count, worst);
    return 0;
}
