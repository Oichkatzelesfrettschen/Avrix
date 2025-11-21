/* SPDX-License-Identifier: MIT */

/**
 * @file signals_pse54.c
 * @brief PSE54 Signal Handling Demo
 *
 * Demonstrates POSIX signal handling in the high-end profile:
 * - Signal registration (sigaction)
 * - Signal masking (sigprocmask)
 * - Signal delivery (kill, raise)
 * - Asynchronous event handling
 *
 * Target: High-end embedded Linux (ARM Cortex-A)
 * Profile: PSE54
 */

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

static volatile int g_signal_count = 0;
static volatile int g_usr1_received = 0;

/**
 * @brief SIGINT handler
 */
static void handle_sigint(int signum) {
    (void)signum;
    printf("\n[Signal] Caught SIGINT (Ctrl+C)\n");
    g_signal_count++;
}

/**
 * @brief SIGUSR1 handler
 */
static void handle_sigusr1(int signum) {
    (void)signum;
    printf("[Signal] Caught SIGUSR1\n");
    g_usr1_received = 1;
}

/**
 * @brief SIGTERM handler
 */
static void handle_sigterm(int signum) {
    (void)signum;
    printf("[Signal] Caught SIGTERM - Exiting safely\n");
    exit(0);
}

int main(void) {
    printf("=== PSE54 Signal Handling Demo ===\n");
    printf("Profile: PSE54 (Full POSIX Signals)\n\n");

    /* 1. Register SIGINT handler */
    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == 0) {
        printf("  ✓ Registered SIGINT handler\n");
    } else {
        perror("sigaction(SIGINT)");
        return 1;
    }

    /* 2. Register SIGUSR1 handler */
    struct sigaction sa_usr1;
    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == 0) {
        printf("  ✓ Registered SIGUSR1 handler\n");
    } else {
        perror("sigaction(SIGUSR1)");
        return 1;
    }

    /* 3. Register SIGTERM handler */
    struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = handle_sigterm;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;

    if (sigaction(SIGTERM, &sa_term, NULL) == 0) {
        printf("  ✓ Registered SIGTERM handler\n");
    } else {
        perror("sigaction(SIGTERM)");
        return 1;
    }

    printf("\nSignal Masking Test:\n");
    printf("--------------------\n");

    /* 4. Block SIGUSR1 temporarily */
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGUSR1);

    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == 0) {
        printf("  ✓ SIGUSR1 blocked (masked)\n");
    }

    printf("  Sending SIGUSR1 to self (should be pending)...\n");
    kill(getpid(), SIGUSR1);

    if (g_usr1_received == 0) {
        printf("  ✓ Signal deferred (masking working)\n");
    } else {
        printf("  ✗ Signal delivered immediately (masking failed)\n");
    }

    printf("  Unblocking SIGUSR1...\n");
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) == 0) {
        printf("  ✓ SIGUSR1 unblocked\n");
    }

    /* Should have been delivered now */
    if (g_usr1_received) {
        printf("  ✓ Signal delivered after unmask\n");
    } else {
        printf("  ✗ Signal lost or not delivered\n");
    }
    printf("\n");

    printf("Waiting for signals (Press Ctrl+C, or wait for timeout)...\n");

    /* Main loop */
    for (int i = 0; i < 5; i++) {
        printf("  Tick %d...\n", i + 1);
        sleep(1);

        if (g_signal_count >= 3) {
            printf("Received 3 SIGINTs, exiting.\n");
            break;
        }
    }

    printf("\nDemo complete.\n");
    return 0;
}
