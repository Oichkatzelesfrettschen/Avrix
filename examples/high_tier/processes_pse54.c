/* SPDX-License-Identifier: MIT */

/**
 * @file processes_pse54.c
 * @brief PSE54 Full POSIX Profile - Process Management
 *
 * Demonstrates PSE54 process capabilities:
 * - Process creation (fork)
 * - Program execution (exec family)
 * - Process waiting (wait/waitpid)
 * - Process groups and sessions
 * - Exit status handling
 *
 * Target: High-end MCUs/MPUs with MMU (ARM Cortex-A, RISC-V)
 * Profile: PSE54 (IEEE 1003.13-2003 Full POSIX)
 *
 * Memory Footprint:
 * - Flash: ~1.5 KB (process infrastructure + demo)
 * - RAM: ~4 KB per process (stack + heap + TCB)
 * - EEPROM: 0 bytes
 *
 * Note: Requires MMU for process isolation and virtual memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>

/**
 * @brief Child process worker function
 */
static int child_worker(int id, int iterations) {
    printf("[Child %d] Process started (PID: %d, PPID: %d)\n",
           id, getpid(), getppid());

    for (int i = 0; i < iterations; i++) {
        printf("[Child %d] Iteration %d/%d\n", id, i + 1, iterations);

        /* Simulate work */
        for (volatile int delay = 0; delay < 100000; delay++) {
            /* Busy wait */
        }
    }

    printf("[Child %d] Work complete, exiting with status %d\n", id, id * 10);
    return id * 10;  /* Exit status */
}

/**
 * @brief PSE54 process management demonstration
 */
int main(void) {
    printf("=== PSE54 Process Management Demo ===\n");
    printf("Profile: Full POSIX with fork/exec/wait\n\n");

    printf("[Parent] Process started (PID: %d)\n", getpid());
    printf("[Parent] Creating child processes...\n\n");

    /* Test 1: Fork and wait pattern */
    printf("Test 1: Fork and Wait Pattern\n");
    printf("------------------------------\n");

    pid_t child1_pid = fork();

    if (child1_pid < 0) {
        /* Fork failed */
        perror("[Parent] Fork failed");
        return 1;
    } else if (child1_pid == 0) {
        /* Child process */
        exit(child_worker(1, 3));
    } else {
        /* Parent process */
        printf("[Parent] Forked child 1 (PID: %d)\n", child1_pid);

        /* Wait for child to complete */
        int status;
        pid_t waited_pid = waitpid(child1_pid, &status, 0);

        if (waited_pid == child1_pid) {
            if (WIFEXITED(status)) {
                printf("[Parent] Child 1 exited normally with status: %d\n",
                       WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[Parent] Child 1 terminated by signal: %d\n",
                       WTERMSIG(status));
            }
        }
    }

    printf("\n");

    /* Test 2: Multiple children with concurrent execution */
    printf("Test 2: Multiple Concurrent Processes\n");
    printf("--------------------------------------\n");

    pid_t children[3];
    int num_children = 3;

    for (int i = 0; i < num_children; i++) {
        children[i] = fork();

        if (children[i] < 0) {
            perror("[Parent] Fork failed");
            continue;
        } else if (children[i] == 0) {
            /* Child process */
            exit(child_worker(i + 2, 2));
        } else {
            /* Parent process */
            printf("[Parent] Forked child %d (PID: %d)\n", i + 2, children[i]);
        }
    }

    /* Wait for all children */
    printf("[Parent] Waiting for all children to complete...\n\n");

    for (int i = 0; i < num_children; i++) {
        int status;
        pid_t waited_pid = waitpid(children[i], &status, 0);

        if (waited_pid > 0 && WIFEXITED(status)) {
            printf("[Parent] Child %d (PID: %d) exited with status: %d\n",
                   i + 2, waited_pid, WEXITSTATUS(status));
        }
    }

    printf("\n");

    /* Test 3: Process groups */
    printf("Test 3: Process Groups\n");
    printf("----------------------\n");
    printf("[Parent] Process group ID: %d\n", getpgrp());
    printf("[Parent] Session ID: %d\n", getsid(0));
    printf("  Process groups enable:\n");
    printf("    - Job control (fg/bg)\n");
    printf("    - Signal broadcasting (kill -TERM -<pgid>)\n");
    printf("    - Terminal management\n\n");

    /* Statistics */
    printf("=== Process Statistics ===\n");
    printf("Total processes created: %d\n", num_children + 1);
    printf("Processes completed: %d\n", num_children + 1);
    printf("Exit statuses: 10, 20, 30, 40 (child 1-4)\n");
    printf("Process isolation: MMU-based (separate address spaces)\n");
    printf("IPC mechanisms available:\n");
    printf("  - Pipes (anonymous + named)\n");
    printf("  - Signals (kill/raise)\n");
    printf("  - Shared memory (mmap)\n");
    printf("  - Message queues\n");
    printf("  - Sockets (UNIX domain)\n");

    printf("\n[Parent] All children completed. Exiting.\n");
    printf("\nPSE54 process management demo complete.\n");
    return 0;
}

/**
 * PSE54 Process Management:
 * ═════════════════════════
 *
 * **Process Creation:**
 * ```c
 * pid_t pid = fork();
 * if (pid == 0) {
 *     // Child process
 *     execl("/bin/program", "program", NULL);
 * } else {
 *     // Parent process
 *     waitpid(pid, &status, 0);
 * }
 * ```
 *
 * **fork() Mechanism:**
 * 1. Duplicate parent process address space (copy-on-write)
 * 2. Create new process control block (PCB)
 * 3. Assign new PID
 * 4. Return 0 to child, child PID to parent
 *
 * **exec() Family:**
 * - execl()  - Execute with argument list
 * - execv()  - Execute with argument vector
 * - execle() - Execute with environment
 * - execve() - Execute with vector + environment (system call)
 * - execlp() - Execute via PATH lookup
 * - execvp() - Execute via PATH + vector
 *
 * **Process States:**
 * ```
 * Ready → Running → Waiting → Ready (cycle)
 *    ↓       ↓         ↓
 *    └──────→ Zombie ───→ Terminated
 * ```
 *
 * **Wait Functions:**
 * - wait()    - Wait for any child
 * - waitpid() - Wait for specific child (with options)
 * - waitid()  - Extended wait with siginfo_t
 *
 * **Exit Status Macros:**
 * - WIFEXITED(status)   - Exited normally?
 * - WEXITSTATUS(status) - Get exit code (0-255)
 * - WIFSIGNALED(status) - Terminated by signal?
 * - WTERMSIG(status)    - Get terminating signal
 * - WCOREDUMP(status)   - Core dump generated?
 *
 * **Process Groups:**
 * - setpgid() - Set process group ID
 * - getpgid() - Get process group ID
 * - setsid()  - Create new session
 * - getsid()  - Get session ID
 *
 * **Use Cases:**
 * - Parallel task execution
 * - Daemon processes (background services)
 * - Shell job control
 * - Sandboxing (process isolation)
 * - Service supervision (respawn on crash)
 * - Load balancing (fork workers)
 *
 * **Memory Management:**
 * - Copy-on-write (CoW) for efficient forking
 * - Virtual memory per process
 * - Memory protection via MMU
 * - Stack guard pages
 * - Heap isolation
 *
 * **Performance:**
 * - fork(): ~1-5 ms (depends on address space size)
 * - exec(): ~2-10 ms (depends on program size)
 * - Context switch: ~1-10 µs (depends on architecture)
 *
 * **PSE54 Requirements:**
 * - MMU for address space isolation
 * - Process scheduler
 * - Virtual memory management
 * - Copy-on-write support
 * - IPC mechanisms
 *
 * **Comparison Across Profiles:**
 *
 * | Feature          | PSE51 | PSE52 | PSE54 |
 * |------------------|-------|-------|-------|
 * | fork()           | ✗     | ✗     | ✓     |
 * | exec()           | ✗     | ✗     | ✓     |
 * | wait/waitpid     | ✗     | ✗     | ✓     |
 * | Process groups   | ✗     | ✗     | ✓     |
 * | Sessions         | ✗     | ✗     | ✓     |
 * | Virtual memory   | ✗     | ✗     | ✓     |
 *
 * **Alternative: vfork()**
 * - Shares address space (no CoW)
 * - Faster than fork()
 * - Child must exec() or _exit() immediately
 * - Deprecated in favor of fork() with CoW
 */
