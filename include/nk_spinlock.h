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
#include <stdatomic.h>

#ifdef __x86_64__
#include <x86intrin.h>
#endif

/*─────────────────────────────── Globals ──────────────────────────────*/
static volatile atomic_ulong tick_count = ATOMIC_VAR_INIT(0);

/*─────────────────────── 1 kHz Timer Interrupt ────────────────────────*/
static void on_tick(int signum)
{
    (void)signum;  // Signal number is unused
    atomic_fetch_add_explicit(&tick_count, 1, memory_order_relaxed);
}

/*────────────────────── High-Resolution Timer ─────────────────────────*/
static inline uint64_t rdcycles(void)
{
#if defined(__i386__) || defined(__x86_64__)
    // Use processor's Time Stamp Counter (TSC)
    return __rdtsc();
#else
    // Fallback to POSIX clock_gettime for portability
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
#endif
}

/*────────────────────── Timer Initialization ──────────────────────────*/
static void init_1khz_timer(void)
{
    struct sigaction sa = { .sa_handler = on_tick };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Restart interrupted syscalls

    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    struct itimerval timer_spec = {
        .it_interval = { .tv_sec = 0, .tv_usec = 1000 }, // 1ms period
        .it_value    = { .tv_sec = 0, .tv_usec = 1000 }
    };

    if (setitimer(ITIMER_REAL, &timer_spec, NULL) < 0) {
        perror("setitimer failed");
        exit(EXIT_FAILURE);
    }
}

/*───────────────────── Spinlock Benchmark & Validation ────────────────*/
int main(void)
{
    // Initialize 1 kHz interrupt-driven timing mechanism
    init_1khz_timer();

    // Spinlock global and local initialization
    nk_spinlock_global_init(); // Ensure global resources (e.g., nk_bkl)
    nk_spinlock_t lock = NK_SPINLOCK_STATIC_INIT;
    nk_spinlock_init(&lock);

    /*──────────────────────── Functional Validation ────────────────────*/

    // Test 1: Blocking Lock/Unlock Semantics
    nk_spinlock_lock(&lock, 1u);
    assert(lock.dag_mask == 1u && "dag_mask mismatch on lock");
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u && "dag_mask not cleared after unlock");

    // Test 2: Non-blocking (Trylock) semantics
    bool lock_acquired = nk_spinlock_trylock(&lock, 2u);
    assert(lock_acquired && lock.dag_mask == 2u && "Trylock failed");
    nk_spinlock_unlock(&lock);
    assert(lock.dag_mask == 0u && "dag_mask not cleared after try-unlock");

    // Test 3: Real-time (RT) blocking semantics
    nk_spinlock_lock_rt(&lock, 3u);
    assert(lock.rt_mode == 1u && lock.dag_mask == 3u && "RT lock incorrect");
    nk_spinlock_unlock_rt(&lock);
    assert(lock.rt_mode == 0u && "RT mode not cleared after unlock");

    // Test 4: Snapshot (Cap’n Proto-style) serialization check
    nk_spinlock_matrix_set(&lock, 0, 0xDEADBEEFu);
    nk_spinlock_capnp_t snap;
    nk_spinlock_encode(&lock, &snap);
    assert(snap.matrix[0] == 0xDEADBEEF && "Snapshot serialization error");

    /*──────────────────────── Benchmark Loop ───────────────────────────*/
    const unsigned loops = 2'000'000u;  // Two million benchmark iterations
    void *begin_brk = sbrk(0);          // Memory boundary verification
    uint64_t worst_cycle_duration = 0;  // Track worst observed cycle latency

    for (unsigned i = 0; i < loops; ++i) {
        uint64_t start_cycles = rdcycles();

        // Benchmark real-time lock/unlock latency
        nk_spinlock_lock_rt(&lock, 0u);
        nk_spinlock_unlock_rt(&lock);

        uint64_t elapsed_cycles = rdcycles() - start_cycles;

        if (elapsed_cycles > worst_cycle_duration) {
            worst_cycle_duration = elapsed_cycles; // Update worst case
        }
    }

    void *end_brk = sbrk(0);
    // Verify no dynamic heap memory growth occurred during benchmark
    assert(begin_brk == end_brk && "Heap expanded unexpectedly");

    /*───────────────────────────── Results ─────────────────────────────*/
    printf("\n─── Spinlock Benchmark Results ───\n");
    printf("Total Benchmark Iterations : %u\n", loops);
    printf("1 kHz Timer Ticks Recorded : %lu\n", atomic_load(&tick_count));
    printf("Worst-case cycle latency   : %" PRIu64 " cycles\n", worst_cycle_duration);
    printf("Memory Stability Check     : PASSED (no heap growth)\n\n");

    return EXIT_SUCCESS;
}
