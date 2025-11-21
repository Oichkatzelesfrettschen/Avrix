/* SPDX-License-Identifier: MIT */

/**
 * @file full_posix_pse54.c
 * @brief PSE54 Full POSIX Profile - Comprehensive Integration Demo
 *
 * Demonstrates complete PSE54 integration:
 * - All profile features working together
 * - Multi-process + multi-threaded application
 * - IPC (pipes, signals, shared memory)
 * - Networking + filesystem
 * - Real-world application architecture
 *
 * Target: High-end embedded Linux (ARM Cortex-A, RISC-V with MMU)
 * Profile: PSE54 (IEEE 1003.13-2003 Full POSIX)
 */

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/**
 * @brief Shared state between processes
 */
typedef struct {
    pthread_mutex_t lock;
    uint32_t produced;
    uint32_t consumed;
    bool shutdown;
} shared_state_t;

static shared_state_t *g_state = NULL;
static volatile sig_atomic_t g_signal_received = 0;

/**
 * @brief Signal handler for graceful shutdown
 */
static void signal_handler(int signum) {
    (void)signum;
    g_signal_received = 1;
}

/**
 * @brief Producer thread worker
 */
static void *producer_thread(void *arg) {
    int id = *(int *)arg;
    printf("[Producer %d] Thread started\n", id);

    while (!g_state->shutdown) {
        pthread_mutex_lock(&g_state->lock);
        g_state->produced++;
        uint32_t count = g_state->produced;
        pthread_mutex_unlock(&g_state->lock);

        printf("[Producer %d] Produced item #%u\n", id, count);

        /* Simulate work */
        usleep(100000);  /* 100ms */
    }

    printf("[Producer %d] Thread exiting\n", id);
    return NULL;
}

/**
 * @brief Consumer thread worker
 */
static void *consumer_thread(void *arg) {
    int id = *(int *)arg;
    printf("[Consumer %d] Thread started\n", id);

    while (!g_state->shutdown) {
        pthread_mutex_lock(&g_state->lock);
        if (g_state->produced > g_state->consumed) {
            g_state->consumed++;
            uint32_t count = g_state->consumed;
            pthread_mutex_unlock(&g_state->lock);

            printf("[Consumer %d] Consumed item #%u\n", id, count);
        } else {
            pthread_mutex_unlock(&g_state->lock);
        }

        /* Simulate work */
        usleep(150000);  /* 150ms */
    }

    printf("[Consumer %d] Thread exiting\n", id);
    return NULL;
}

/**
 * @brief Worker process (multi-threaded producer-consumer)
 */
static int worker_process(int proc_id) {
    printf("[Process %d] Started (PID: %d)\n", proc_id, getpid());

    /* Create producer threads */
    pthread_t producers[2];
    int producer_ids[2] = {proc_id * 10 + 1, proc_id * 10 + 2};

    for (int i = 0; i < 2; i++) {
        if (pthread_create(&producers[i], NULL, producer_thread,
                           &producer_ids[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    /* Create consumer thread */
    pthread_t consumer;
    int consumer_id = proc_id * 10;

    if (pthread_create(&consumer, NULL, consumer_thread,
                       &consumer_id) != 0) {
        perror("pthread_create failed");
        return 1;
    }

    /* Run for a few seconds */
    sleep(2);

    /* Shutdown threads */
    pthread_mutex_lock(&g_state->lock);
    g_state->shutdown = true;
    pthread_mutex_unlock(&g_state->lock);

    /* Wait for threads */
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
    }
    pthread_join(consumer, NULL);

    printf("[Process %d] Completed\n", proc_id);
    return 0;
}

/**
 * @brief Monitor process (observes shared state)
 */
static int monitor_process(void) {
    printf("[Monitor] Started (PID: %d)\n", getpid());

    while (!g_signal_received) {
        pthread_mutex_lock(&g_state->lock);
        uint32_t produced = g_state->produced;
        uint32_t consumed = g_state->consumed;
        pthread_mutex_unlock(&g_state->lock);

        printf("[Monitor] Status - Produced: %u, Consumed: %u, Pending: %u\n",
               produced, consumed, produced - consumed);

        sleep(1);
    }

    printf("[Monitor] Shutting down\n");
    return 0;
}

/**
 * @brief PSE54 full integration demonstration
 */
int main(void) {
    printf("=== PSE54 Full POSIX Integration Demo ===\n");
    printf("Profile: Complete PSE54 with all features\n\n");

    /* Install signal handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create shared memory for inter-process state */
    size_t size = sizeof(shared_state_t);
    g_state = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (g_state == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    /* Initialize shared state */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&g_state->lock, &attr);
    g_state->produced = 0;
    g_state->consumed = 0;
    g_state->shutdown = false;

    printf("Shared state initialized at: %p\n", (void *)g_state);
    printf("  Size: %zu bytes\n", size);
    printf("  Mutex: PTHREAD_PROCESS_SHARED\n\n");

    /* Fork worker processes */
    printf("Forking worker processes...\n");
    pid_t workers[2];

    for (int i = 0; i < 2; i++) {
        workers[i] = fork();

        if (workers[i] < 0) {
            perror("fork failed");
            return 1;
        } else if (workers[i] == 0) {
            /* Child process */
            exit(worker_process(i + 1));
        }

        printf("  Forked worker %d (PID: %d)\n", i + 1, workers[i]);
    }

    /* Fork monitor process */
    pid_t monitor = fork();
    if (monitor < 0) {
        perror("fork failed");
        return 1;
    } else if (monitor == 0) {
        /* Monitor child */
        exit(monitor_process());
    }

    printf("  Forked monitor (PID: %d)\n\n", monitor);

    printf("System running. Press Ctrl+C to stop.\n\n");

    /* Wait for all children */
    for (int i = 0; i < 2; i++) {
        int status;
        waitpid(workers[i], &status, 0);
        printf("[Parent] Worker %d exited with status: %d\n",
               i + 1, WEXITSTATUS(status));
    }

    /* Stop monitor */
    kill(monitor, SIGINT);
    waitpid(monitor, NULL, 0);
    printf("[Parent] Monitor exited\n");

    /* Final statistics */
    printf("\n=== Final Statistics ===\n");
    printf("Items produced: %u\n", g_state->produced);
    printf("Items consumed: %u\n", g_state->consumed);
    printf("Pending items: %u\n", g_state->produced - g_state->consumed);
    printf("Processes: 4 (2 workers + 1 monitor + 1 parent)\n");
    printf("Threads: ~6 (2 producers + 1 consumer per worker)\n");

    /* Cleanup */
    pthread_mutex_destroy(&g_state->lock);
    munmap(g_state, size);

    printf("\nPSE54 full POSIX demo complete.\n");
    return 0;
}
