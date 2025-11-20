/* SPDX-License-Identifier: MIT */

/**
 * @file signals_pse54.c
 * @brief PSE54 Full POSIX Profile - Signal Handling
 *
 * Demonstrates PSE54 signal capabilities:
 * - POSIX signals (SIGINT, SIGTERM, SIGUSR1, etc.)
 * - Signal handlers (sigaction)
 * - Signal masks (sigprocmask)
 * - Asynchronous event notification
 *
 * Target: High-end MCUs (ARM Cortex-A, RISC-V with MMU)
 * Profile: PSE54 (IEEE 1003.13-2003 Full POSIX)
 *
 * Memory Footprint:
 * - Flash: ~1.2 KB (signal infrastructure + demo)
 * - RAM: ~256 bytes (signal handlers + state)
 * - EEPROM: 0 bytes
 *
 * Note: PSE54 requires hardware with memory protection (MMU/MPU)
 * for proper signal isolation.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

/**
 * @brief Application state
 */
static volatile sig_atomic_t g_signal_count = 0;
static volatile bool g_running = true;

/**
 * @brief Signal handler for SIGINT (Ctrl+C)
 */
static void sigint_handler(int signum) {
    (void)signum;  /* Unused */

    printf("\n[Signal Handler] Caught SIGINT (Ctrl+C)\n");
    printf("  Signal number: %d\n", SIGINT);
    printf("  Count: %d\n", ++g_signal_count);

    if (g_signal_count >= 3) {
        printf("  Terminating after 3 signals...\n");
        g_running = false;
    } else {
        printf("  Press Ctrl+C %d more time(s) to exit\n", 3 - g_signal_count);
    }
}

/**
 * @brief Signal handler for SIGUSR1 (user-defined signal 1)
 */
static void sigusr1_handler(int signum) {
    (void)signum;

    printf("\n[Signal Handler] Caught SIGUSR1\n");
    printf("  Custom signal for application-specific events\n");
    printf("  Example: Configuration reload triggered\n");
}

/**
 * @brief Signal handler for SIGTERM (termination request)
 */
static void sigterm_handler(int signum) {
    (void)signum;

    printf("\n[Signal Handler] Caught SIGTERM\n");
    printf("  Graceful shutdown requested\n");
    printf("  Cleaning up resources...\n");
    g_running = false;
}

/**
 * @brief PSE54 signal handling demonstration
 */
int main(void) {
    printf("=== PSE54 Signal Handling Demo ===\n");
    printf("Profile: Full POSIX with asynchronous signals\n\n");

    /* Install signal handlers (PSE54: sigaction) */
    printf("Installing signal handlers...\n");

    struct sigaction sa_int = {0};
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == 0) {
        printf("  ✓ SIGINT handler installed\n");
    } else {
        printf("  ✗ SIGINT handler failed\n");
        return 1;
    }

    struct sigaction sa_usr1 = {0};
    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == 0) {
        printf("  ✓ SIGUSR1 handler installed\n");
    } else {
        printf("  ✗ SIGUSR1 handler failed\n");
    }

    struct sigaction sa_term = {0};
    sa_term.sa_handler = sigterm_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;

    if (sigaction(SIGTERM, &sa_term, NULL) == 0) {
        printf("  ✓ SIGTERM handler installed\n");
    } else {
        printf("  ✗ SIGTERM handler failed\n");
    }

    printf("\n");

    /* Demonstrate signal masking */
    printf("Demonstrating signal masking:\n");

    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGUSR1);

    printf("  Blocking SIGUSR1...\n");
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == 0) {
        printf("  ✓ SIGUSR1 blocked (will be queued if raised)\n");
    }

    printf("  Unblocking SIGUSR1...\n");
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) == 0) {
        printf("  ✓ SIGUSR1 unblocked (pending signals delivered)\n");
    }

    printf("\n");

    /* Main event loop */
    printf("Main event loop running...\n");
    printf("  Press Ctrl+C to trigger SIGINT (3 times to exit)\n");
    printf("  Send SIGUSR1 for custom event (kill -USR1 <pid>)\n");
    printf("  Send SIGTERM for graceful shutdown (kill -TERM <pid>)\n\n");

    uint32_t iteration = 0;
    while (g_running) {
        /* Simulate work */
        iteration++;

        if (iteration % 100000 == 0) {
            printf("[Main Loop] Iteration: %lu (signal count: %d)\n",
                   iteration, g_signal_count);
        }

        /* In real application:
         * - Process network packets
         * - Handle I/O events
         * - Update sensors
         * - etc.
         */
    }

    printf("\n[Main Loop] Exiting cleanly\n");

    /* Statistics */
    printf("\n=== Signal Statistics ===\n");
    printf("Total signals received: %d\n", g_signal_count);
    printf("Handlers installed: 3 (SIGINT, SIGUSR1, SIGTERM)\n");
    printf("Iterations completed: %lu\n", iteration);
    printf("Exit reason: %s\n",
           g_signal_count >= 3 ? "SIGINT (user)" : "SIGTERM (graceful)");

    printf("\nPSE54 signal handling demo complete.\n");
    return 0;
}

/**
 * PSE54 Signal Handling:
 * ══════════════════════
 *
 * **POSIX Signals Supported:**
 * - SIGINT  (2)  - Interrupt (Ctrl+C)
 * - SIGTERM (15) - Termination request
 * - SIGUSR1 (10) - User-defined signal 1
 * - SIGUSR2 (12) - User-defined signal 2
 * - SIGALRM (14) - Alarm clock timer
 * - SIGCHLD (17) - Child process status change
 * - SIGHUP  (1)  - Hangup (terminal disconnect)
 * - SIGQUIT (3)  - Quit (Ctrl+\)
 * - SIGKILL (9)  - Kill (uncatchable)
 * - SIGSTOP (19) - Stop (uncatchable)
 *
 * **Signal Delivery Mechanism:**
 * 1. Hardware/software raises signal (e.g., Ctrl+C → SIGINT)
 * 2. Kernel checks signal mask (blocked vs pending)
 * 3. If not blocked, kernel invokes registered handler
 * 4. Handler executes in interrupted context
 * 5. Return from handler resumes interrupted code
 *
 * **Signal Safety:**
 * Only async-signal-safe functions can be called in handlers:
 * - write() - YES (safe)
 * - printf() - NO (unsafe, but used in demo for clarity)
 * - malloc() - NO (unsafe)
 * - pthread_mutex_lock() - NO (unsafe)
 *
 * Safe pattern: Set volatile sig_atomic_t flag, check in main loop.
 *
 * **sigaction vs signal:**
 * - signal(): Legacy, unreliable (resets handler after call)
 * - sigaction(): Modern, reliable (persistent handler)
 * Always use sigaction() in PSE54!
 *
 * **Signal Masking:**
 * ```c
 * sigset_t set;
 * sigemptyset(&set);
 * sigaddset(&set, SIGUSR1);
 * sigprocmask(SIG_BLOCK, &set, NULL);    // Block SIGUSR1
 * // Critical section here
 * sigprocmask(SIG_UNBLOCK, &set, NULL);  // Unblock
 * ```
 *
 * **Real-Time Signals (POSIX.1b):**
 * - SIGRTMIN to SIGRTMAX (34-64)
 * - Queued (unlike standard signals)
 * - Delivered in priority order
 * - Can carry additional data (sigqueue)
 *
 * **Use Cases:**
 * - Graceful shutdown (SIGTERM)
 * - Configuration reload (SIGUSR1)
 * - Watchdog timer (SIGALRM)
 * - Child process management (SIGCHLD)
 * - Inter-process notification
 * - Asynchronous I/O completion
 *
 * **PSE54 Requirements:**
 * - Memory protection (MMU/MPU) for signal isolation
 * - Process model (fork/exec)
 * - Signal queuing infrastructure
 * - Async-signal-safe system calls
 *
 * **Performance:**
 * - Signal delivery: ~100-500 cycles (context switch + handler)
 * - Handler overhead: Depends on handler complexity
 * - Signal masking: ~10 cycles (bit manipulation)
 *
 * **Comparison Across Profiles:**
 *
 * | Feature          | PSE51 | PSE52 | PSE54 |
 * |------------------|-------|-------|-------|
 * | Signals          | ✗     | ✗     | ✓     |
 * | sigaction        | ✗     | ✗     | ✓     |
 * | Signal masking   | ✗     | ✗     | ✓     |
 * | Real-time signals| ✗     | ✗     | ✓     |
 * | kill/raise       | ✗     | ✗     | ✓     |
 */
