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
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    struct itimerval tv = {
        .it_interval = { .tv_sec = 0, .tv_usec = 1000 },
        .it_value    = { .tv_sec = 0, .tv_usec = 1000 }
    };
    if (setitimer(ITIMER_REAL, &tv, NULL) < 0) {
        perror("setitimer");
        return EXIT_FAILURE;
    }

    /*────────────────── Initialize unified spinlock ─────────────────────*/
    nk_spinlock_global_init(); /* ensure nk_bkl is ready */
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    /*────────────────── Functional self-checks ─────────────────────────*/
    /* 1) Blocking lock/unlock */
    nk_spinlock_lock(&lock, 1u);
    assert(lock.dag_mask == 1u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* 2) Trylock/unlock */
    bool ok = nk_spinlock_trylock(&lock, 2u);
    assert(ok && lock.dag_mask == 2u);
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u);

    /* 3) Real-time blocking lock/unlock */
    nk_spinlock_lock_rt(&lock, 3u);
    assert(lock.rt_mode == 1u && lock.dag_mask == 3u);
    nk_spinlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u);

    /* 4) Snapshot matrix encode/decode */
    nk_spinlock_matrix_set(&lock, 0, 0xDEADBEEFu);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.matrix[0] == 0xDEADBEEF);

    /*───────────────────────── Benchmark loop ──────────────────────────*/
    const unsigned loops = 2000000u;
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
    printf("ticks: %lu\n", tick_count);
    printf("worst cycles: %" PRIu64 "\n", worst);

    return 0;
}
