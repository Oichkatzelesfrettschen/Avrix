#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "nk_lock.h"
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#ifdef __x86_64__
#include <x86intrin.h>
#endif

/* Simple 1 kHz timer interrupt counter */
static volatile unsigned long tick_count = 0;

static void on_tick(int signum)
{
    (void)signum;
    ++tick_count;
}

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
    struct sigaction sa = {0};
    sa.sa_handler = on_tick;
    sigaction(SIGALRM, &sa, NULL);

    /* fire every millisecond */
    struct itimerval tv = { {0, 1000}, {0, 1000} };
    setitimer(ITIMER_REAL, &tv, NULL);

    nk_slock_t lock;
    nk_slock_init(&lock);

    const unsigned loops = 2000000u; /* two million iterations */
    void *begin_brk = sbrk(0);
    uint64_t worst = 0;

    for (unsigned i = 0; i < loops; ++i) {
        uint64_t t0 = rdcycles();
        nk_slock_lock(&lock);
        nk_slock_unlock(&lock);
        uint64_t dt = rdcycles() - t0;
        if (dt > worst)
            worst = dt;
    }

    void *end_brk = sbrk(0);
    /* lock operations should not allocate memory */
    assert(begin_brk == end_brk);

    printf("ticks: %lu\nworst cycles: %" PRIu64 "\n", tick_count, worst);
    return 0;
}
