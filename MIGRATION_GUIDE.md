# Avrix Migration Guide

**Upgrading Between POSIX Profiles:** PSE51 → PSE52 → PSE54

**Target Audience:** Developers migrating applications between POSIX profiles

---

## Table of Contents

1. [Overview](#overview)
2. [PSE51 → PSE52 Migration](#pse51--pse52-migration)
3. [PSE52 → PSE54 Migration](#pse52--pse54-migration)
4. [API Changes](#api-changes)
5. [Code Examples](#code-examples)

---

## Overview

Avrix supports three POSIX profiles (IEEE Std 1003.13-2003):

| Profile | Description | Target Hardware |
|---------|-------------|-----------------|
| **PSE51** | Minimal (single-threaded) | 8-16KB flash, 1-2KB RAM |
| **PSE52** | Multi-threaded (pthread) | 32-128KB flash, 4-16KB RAM |
| **PSE54** | Full POSIX (processes + MMU) | 128KB+ flash, 16MB+ RAM |

### Migration Paths

```
PSE51 (Minimal)
  ↓ Add pthread, networking
PSE52 (Multi-threaded)
  ↓ Add processes, signals, MMU
PSE54 (Full POSIX)
```

---

## PSE51 → PSE52 Migration

### Hardware Requirements
- **Flash**: 8-16KB → 32-128KB (4x increase)
- **RAM**: 1-2KB → 4-16KB (4x increase)
- **Timer**: Add 1 kHz tick timer for scheduler

### Feature Additions
| Feature | PSE51 | PSE52 |
|---------|-------|-------|
| Threading (pthread) | ✗ | ✓ |
| Preemptive scheduling | ✗ | ✓ |
| Mutex/semaphores | ✗ | ✓ |
| Networking (SLIP/IPv4) | ✗ | ✓ |
| IPC (Door RPC) | ✗ | ✓ |
| VFS (unified filesystem) | ✗ | ✓ |

### Step 1: Update Cooperative Tasks → pthread

**PSE51 (Cooperative):**
```c
// Old: Cooperative task scheduler
typedef struct {
    uint32_t next_run_ms;
    uint32_t interval_ms;
    bool enabled;
} task_t;

void main_loop(void) {
    task_t tasks[3] = {...};

    while (1) {
        uint32_t now = get_time_ms();

        for (int i = 0; i < 3; i++) {
            if (tasks[i].enabled && now >= tasks[i].next_run_ms) {
                task_function[i]();  // Execute task
                tasks[i].next_run_ms = now + tasks[i].interval_ms;
            }
        }
    }
}
```

**PSE52 (Preemptive pthread):**
```c
// New: pthread-based concurrent execution
void *task1_thread(void *arg) {
    while (1) {
        // Task 1 work
        usleep(500000);  // 500ms
    }
    return NULL;
}

void *task2_thread(void *arg) {
    while (1) {
        // Task 2 work
        usleep(1000000);  // 1000ms
    }
    return NULL;
}

int main(void) {
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, task1_thread, NULL);
    pthread_create(&tid2, NULL, task2_thread, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}
```

**Migration Checklist:**
- [ ] Allocate stack per thread (128-256 bytes)
- [ ] Convert task functions to `void *(*)(void *)`
- [ ] Replace polling loops with `usleep()`/`nanosleep()`
- [ ] Add thread creation in `main()`

### Step 2: Add Synchronization

**PSE51 (No synchronization):**
```c
// Old: Global variables (unsafe with interrupts)
uint32_t g_counter = 0;

void task1(void) {
    g_counter++;  // Race condition with ISR!
}

ISR(TIMER0_COMPA_vect) {
    g_counter++;  // Race condition!
}
```

**PSE52 (Mutex protection):**
```c
// New: Mutex-protected shared state
uint32_t g_counter = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void *task1(void *arg) {
    while (1) {
        pthread_mutex_lock(&g_mutex);
        g_counter++;
        pthread_mutex_unlock(&g_mutex);

        usleep(100000);
    }
    return NULL;
}

// Note: ISRs still use atomic ops, not mutexes
ISR(TIMER0_COMPA_vect) {
    __sync_fetch_and_add(&g_counter, 1);  // Atomic increment
}
```

**Migration Checklist:**
- [ ] Identify shared variables
- [ ] Add `pthread_mutex_t` for each shared resource
- [ ] Wrap accesses with `pthread_mutex_lock/unlock`
- [ ] Use atomics in ISRs (not mutexes!)

### Step 3: Add Networking (Optional)

**PSE51 (No networking):**
```c
// Old: Printf-based output
printf("Sensor: %d\n", value);
```

**PSE52 (SLIP + IPv4):**
```c
// New: Network transmission
#include "drivers/tty/tty.h"
#include "drivers/net/slip.h"
#include "drivers/net/ipv4.h"

tty_t serial;
uint8_t rx_buf[64], tx_buf[64];

// Initialize
tty_init(&serial, rx_buf, tx_buf, 64, uart_putc, uart_getc);

// Send sensor data over network
void send_sensor_data(uint16_t value) {
    ipv4_hdr_t hdr;
    ipv4_init_header(&hdr, LOCAL_IP, REMOTE_IP, IPV4_PROTO_UDP, sizeof(value));

    ipv4_send(&serial, &hdr, &value, sizeof(value));
}
```

**Migration Checklist:**
- [ ] Add TTY driver initialization
- [ ] Configure IP addresses
- [ ] Replace `printf()` with `ipv4_send()`

### Step 4: Add VFS (Optional)

**PSE51 (Direct filesystem access):**
```c
// Old: Direct ROMFS/EEPFS calls
const romfs_file_t *file = romfs_open("config.txt");
romfs_read(file, 0, buffer, size);

const eepfs_file_t *log = eepfs_open("data.log");
eepfs_write(log, 0, data, len);
```

**PSE52 (Unified VFS):**
```c
// New: POSIX-like VFS API
#include "drivers/fs/vfs.h"

// Mount filesystems
vfs_mount(VFS_TYPE_ROMFS, "/rom");
vfs_mount(VFS_TYPE_EEPFS, "/eeprom");

// Unified file access
int fd = vfs_open("/rom/config.txt", VFS_O_RDONLY);
vfs_read(fd, buffer, size);
vfs_close(fd);

fd = vfs_open("/eeprom/data.log", VFS_O_RDWR);
vfs_write(fd, data, len);
vfs_close(fd);
```

**Migration Checklist:**
- [ ] Add `vfs_mount()` calls in `main()`
- [ ] Replace filesystem-specific APIs with `vfs_*`
- [ ] Update file paths to include mount points

---

## PSE52 → PSE54 Migration

### Hardware Requirements
- **Flash**: 32-128KB → 128KB+ (minimal change)
- **RAM**: 4-16KB → 16MB+ (1000x increase!)
- **MMU**: Required for virtual memory

### Feature Additions
| Feature | PSE52 | PSE54 |
|---------|-------|-------|
| Processes (fork/exec) | ✗ | ✓ |
| Signals (SIGINT, etc.) | ✗ | ✓ |
| MMU + virtual memory | ✗ | ✓ |
| Process isolation | ✗ | ✓ |
| Shared memory (mmap) | ✗ | ✓ |

### Step 1: Convert Threads → Processes

**PSE52 (Threads only):**
```c
// Old: Multi-threaded worker pool
void *worker_thread(void *arg) {
    int id = *(int *)arg;
    while (1) {
        process_task(id);
    }
    return NULL;
}

int main(void) {
    pthread_t workers[4];
    int ids[4] = {1, 2, 3, 4};

    for (int i = 0; i < 4; i++) {
        pthread_create(&workers[i], NULL, worker_thread, &ids[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(workers[i], NULL);
    }
}
```

**PSE54 (Multi-process):**
```c
// New: Multi-process worker pool
int main(void) {
    pid_t workers[4];

    for (int i = 0; i < 4; i++) {
        workers[i] = fork();

        if (workers[i] == 0) {
            // Child process
            int id = i + 1;
            while (1) {
                process_task(id);
            }
            exit(0);
        }
    }

    // Parent waits for children
    for (int i = 0; i < 4; i++) {
        int status;
        waitpid(workers[i], &status, 0);
    }
}
```

**Migration Checklist:**
- [ ] Replace `pthread_create` with `fork`
- [ ] Replace `pthread_join` with `waitpid`
- [ ] Add `exit()` in child processes
- [ ] Handle process termination signals

### Step 2: Add Signal Handling

**PSE52 (No signals):**
```c
// Old: Global flag for shutdown
volatile bool g_shutdown = false;

int main(void) {
    while (!g_shutdown) {
        // Main loop
    }
}

// Shutdown triggered elsewhere
void shutdown_system(void) {
    g_shutdown = true;
}
```

**PSE54 (Signal-based shutdown):**
```c
// New: Signal-driven shutdown
#include <signal.h>

volatile sig_atomic_t g_shutdown = 0;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_shutdown = 1;
    }
}

int main(void) {
    // Install signal handlers
    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while (!g_shutdown) {
        // Main loop
    }

    return 0;
}
```

**Migration Checklist:**
- [ ] Use `sig_atomic_t` for signal-modified variables
- [ ] Install handlers with `sigaction()` (not `signal()`)
- [ ] Only call async-signal-safe functions in handlers

### Step 3: Add Shared Memory IPC

**PSE52 (Thread-shared global):**
```c
// Old: Threads share globals automatically
uint32_t g_shared_counter = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_func(void *arg) {
    pthread_mutex_lock(&g_mutex);
    g_shared_counter++;
    pthread_mutex_unlock(&g_mutex);
    return NULL;
}
```

**PSE54 (Process-shared memory):**
```c
// New: Processes use mmap for shared memory
#include <sys/mman.h>

typedef struct {
    uint32_t counter;
    pthread_mutex_t mutex;
} shared_state_t;

int main(void) {
    // Create shared memory
    shared_state_t *state = mmap(NULL, sizeof(shared_state_t),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Initialize process-shared mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->mutex, &attr);

    state->counter = 0;

    pid_t child = fork();
    if (child == 0) {
        // Child process
        pthread_mutex_lock(&state->mutex);
        state->counter++;
        pthread_mutex_unlock(&state->mutex);
        exit(0);
    }

    waitpid(child, NULL, 0);
    printf("Counter: %u\n", state->counter);

    munmap(state, sizeof(shared_state_t));
}
```

**Migration Checklist:**
- [ ] Replace global variables with `mmap(MAP_SHARED)`
- [ ] Use `PTHREAD_PROCESS_SHARED` for mutexes
- [ ] Add `munmap()` cleanup

---

## API Changes

### Function Mapping

| PSE51 | PSE52 | PSE54 | Notes |
|-------|-------|-------|-------|
| `while(1) { task(); delay(); }` | `pthread_create()` | `fork()` | Concurrency |
| Global variables | `pthread_mutex_t` | `mmap(MAP_SHARED)` | Shared state |
| N/A | `usleep()` | `nanosleep()` | Delays |
| N/A | `slip_send()` | `send()` | Networking |
| `romfs_open()` | `vfs_open()` | `open()` | File I/O |
| N/A | N/A | `sigaction()` | Signals |
| N/A | N/A | `mprotect()` | Memory protection |

### Header Changes

| PSE51 | PSE52 | PSE54 |
|-------|-------|-------|
| `#include <stdio.h>` | `#include <pthread.h>` | `#include <signal.h>` |
| `#include "drivers/fs/romfs.h"` | `#include "drivers/fs/vfs.h"` | `#include <sys/mman.h>` |
| | `#include "drivers/net/slip.h"` | `#include <sys/wait.h>` |

---

## Code Examples

### Example 1: Sensor Logger Migration

**PSE51:**
```c
void main_loop(void) {
    while (1) {
        uint16_t sensor = read_sensor();
        log_to_eeprom(sensor);
        delay_ms(1000);
    }
}
```

**PSE52:**
```c
void *logger_thread(void *arg) {
    int fd = vfs_open("/eeprom/log.dat", VFS_O_WRONLY);
    while (1) {
        uint16_t sensor = read_sensor();
        vfs_write(fd, &sensor, sizeof(sensor));
        usleep(1000000);  // 1 second
    }
    return NULL;
}
```

**PSE54:**
```c
int main(void) {
    pid_t logger = fork();
    if (logger == 0) {
        int fd = open("/eeprom/log.dat", O_WRONLY);
        while (1) {
            uint16_t sensor = read_sensor();
            write(fd, &sensor, sizeof(sensor));
            sleep(1);
        }
    }
    waitpid(logger, NULL, 0);
}
```

---

*Last Updated: Phase 8, Session 2025-01-XX*
