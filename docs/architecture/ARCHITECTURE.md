# Avrix Scalable Embedded POSIX System Architecture

## Executive Summary

Avrix is a modular embedded operating system that provides a POSIX-like environment across microcontrollers ranging from 8-bit (ATmega128*) to 32-bit (ARM Cortex-M). The design philosophy is **bottom-up**: starting from the smallest systems' constraints and building upward with additional functionality for larger MCUs.

**Version:** 2.0 (Scalable POSIX System)
**Date:** 2025-11-19
**Status:** Under Development

---

## 1. Design Principles

### 1.1 Core Tenets

1. **Modularity**: Features are decoupled and can be enabled/disabled at build time
2. **Portability**: Hardware abstraction enables cross-architecture support
3. **Size Optimization**: Aggressive optimization for flash and RAM constraints
4. **POSIX Compliance**: Standard API surface with graceful degradation
5. **Bottom-Up Scaling**: Design from smallest to largest, not vice versa
6. **Zero Runtime Overhead**: All configuration at compile/link time

### 1.2 Three-Tier Grouping

| Tier | MCU Class | RAM | Flash | Profile | POSIX |
|------|-----------|-----|-------|---------|-------|
| **Low** | 8-bit classic | 128B-4KB | 4-32KB | Minimal | PSE51/52 |
| **Mid** | Enhanced 8/16-bit | 4-16KB | 32-128KB | Enhanced | PSE52+ |
| **High** | 32-bit | 16KB-1MB | 128KB-2MB | Full | PSE53/54 |

---

## 2. Directory Structure

### 2.1 New Organization

```
Avrix/
├── arch/                   # Architecture-specific code (HAL)
│   ├── avr8/              # AVR 8-bit (ATmega series)
│   │   ├── atmega128/     # ATmega128 family
│   │   ├── atmega328p/    # ATmega328P family
│   │   ├── common/        # Shared AVR code
│   │   └── include/       # AVR-specific headers
│   ├── armcm/             # ARM Cortex-M
│   │   ├── cortex-m0/
│   │   ├── cortex-m3/
│   │   ├── cortex-m4/
│   │   └── common/
│   ├── msp430/            # TI MSP430
│   └── common/            # Cross-architecture utilities
│
├── kernel/                # Portable OS core
│   ├── sched/            # Scheduler
│   ├── ipc/              # Inter-process communication
│   ├── sync/             # Synchronization primitives
│   ├── mm/               # Memory management
│   └── time/             # Timers and sleep
│
├── lib/                  # Standard libraries
│   ├── posix/           # POSIX API implementation
│   │   ├── unistd/      # unistd.h functions
│   │   ├── pthread/     # pthread API
│   │   ├── stdio/       # Standard I/O
│   │   └── stubs/       # Stubs for unsupported features
│   ├── libc/            # Minimal C library
│   └── util/            # Utility functions
│
├── drivers/             # Hardware drivers
│   ├── dev/            # Character/block devices
│   │   ├── uart/       # UART drivers
│   │   ├── spi/        # SPI drivers
│   │   ├── i2c/        # I2C drivers
│   │   └── gpio/       # GPIO drivers
│   ├── fs/             # Filesystems
│   │   ├── ramfs/      # RAM-based filesystem
│   │   ├── romfs/      # ROM filesystem
│   │   ├── eepfs/      # EEPROM filesystem
│   │   └── vfs/        # Virtual filesystem layer
│   └── net/            # Network drivers
│       ├── slip/       # SLIP protocol
│       └── eth/        # Ethernet (high-end only)
│
├── config/             # Build configurations
│   ├── profiles/       # MCU tier profiles
│   │   ├── low_profile.conf
│   │   ├── mid_profile.conf
│   │   └── high_profile.conf
│   ├── boards/         # Board-specific configs
│   │   ├── arduino_uno/
│   │   ├── arduino_mega/
│   │   └── stm32f4_discovery/
│   └── packages/       # Feature packages
│       ├── filesystem.conf
│       ├── networking.conf
│       └── threading.conf
│
├── src/                # Legacy source (to be migrated)
├── include/            # Legacy headers (to be migrated)
├── examples/           # Example applications
│   ├── low_tier/      # Low-end MCU examples
│   ├── mid_tier/      # Mid-range MCU examples
│   └── high_tier/     # High-end MCU examples
├── tests/             # Unit and integration tests
├── docs/              # Documentation
│   ├── architecture/  # Architecture documentation
│   ├── api/          # API reference
│   └── guides/       # User guides
└── scripts/          # Build and utility scripts
```

### 2.2 Migration Strategy

1. **Phase 1**: Create new directory structure
2. **Phase 2**: Extract AVR-specific code to `arch/avr8/`
3. **Phase 3**: Move portable kernel code to `kernel/`
4. **Phase 4**: Create POSIX API layer in `lib/posix/`
5. **Phase 5**: Reorganize drivers into `drivers/`
6. **Phase 6**: Create configuration system in `config/`
7. **Phase 7**: Update build system
8. **Phase 8**: Migrate examples and tests

---

## 3. Hardware Abstraction Layer (HAL)

### 3.1 HAL Interface

Each architecture implements a common HAL interface:

```c
/* arch/common/hal.h */

// Core system control
void hal_init(void);
void hal_reset(void);
void hal_idle(void);

// Interrupt management
void hal_irq_enable(void);
void hal_irq_disable(void);
bool hal_irq_enabled(void);

// Timer/Clock
void hal_timer_init(uint32_t freq_hz);
uint32_t hal_timer_ticks(void);
void hal_timer_delay_us(uint32_t us);

// Context switching
void hal_context_init(hal_context_t *ctx, void *entry, void *stack, size_t stack_size);
void hal_context_switch(hal_context_t *from, hal_context_t *to);

// Memory barriers
void hal_memory_barrier(void);
void hal_dmb(void);
void hal_dsb(void);

// Atomic operations
uint8_t hal_atomic_exchange_u8(volatile uint8_t *ptr, uint8_t val);
uint16_t hal_atomic_exchange_u16(volatile uint16_t *ptr, uint16_t val);
bool hal_atomic_compare_exchange_u8(volatile uint8_t *ptr, uint8_t *expected, uint8_t val);

// Optional features (return false if not supported)
bool hal_has_mpu(void);
bool hal_has_fpu(void);
bool hal_has_hardware_div(void);
```

### 3.2 AVR8 HAL Implementation

```c
/* arch/avr8/common/hal.c */

void hal_init(void) {
    // Architecture-specific initialization
    // - Set up stack pointer
    // - Initialize peripherals
}

void hal_irq_enable(void) {
    sei();  // AVR-specific
}

void hal_irq_disable(void) {
    cli();  // AVR-specific
}

// ... implementation for all HAL functions
```

### 3.3 Architecture Detection

```c
/* arch/common/arch_detect.h */

#if defined(__AVR__)
    #include "arch/avr8/hal.h"
    #define ARCH_AVR8 1
    #define ARCH_WORD_SIZE 8
#elif defined(__ARM_ARCH)
    #include "arch/armcm/hal.h"
    #define ARCH_ARMCM 1
    #define ARCH_WORD_SIZE 32
#elif defined(__MSP430__)
    #include "arch/msp430/hal.h"
    #define ARCH_MSP430 1
    #define ARCH_WORD_SIZE 16
#else
    #error "Unsupported architecture"
#endif
```

---

## 4. Kernel Architecture

### 4.1 Kernel Subsystems

```
kernel/
├── sched/           # Scheduler
│   ├── scheduler.c  # Core scheduling logic
│   ├── task.c       # Task management
│   └── idle.c       # Idle task
├── ipc/             # Inter-process communication
│   ├── door.c       # Door RPC (inherited from µ-UNIX)
│   ├── pipe.c       # POSIX pipes (optional)
│   └── mqueue.c     # Message queues (optional)
├── sync/            # Synchronization
│   ├── spinlock.c   # Spinlocks
│   ├── mutex.c      # Mutexes
│   ├── semaphore.c  # Semaphores
│   └── rwlock.c     # Read-write locks
├── mm/              # Memory management
│   ├── kalloc.c     # Kernel allocator
│   ├── slab.c       # Slab allocator (optional)
│   └── mpu.c        # MPU support (optional)
└── time/            # Time management
    ├── timer.c      # System timer
    └── sleep.c      # Task sleep
```

### 4.2 Scheduler Design

**Low-End Profile (8-bit, minimal RAM):**
- Cooperative or simple preemptive
- Round-robin only
- Max 4-8 tasks
- Static priority
- No dynamic priority adjustment

**Mid-Range Profile (enhanced 8/16-bit):**
- Preemptive multitasking
- Priority-based scheduling
- Max 8-16 tasks
- Optional time-slicing
- Basic priority inheritance

**High-End Profile (32-bit):**
- Fully preemptive kernel
- Multiple scheduling policies (FIFO, RR, EDF)
- Max 32+ tasks
- Priority inheritance
- SMP support (future)

### 4.3 Memory Management

**Low-End:**
- Static allocation only
- Fixed-size blocks
- No dynamic allocation
- Simple bump allocator if needed

**Mid-Range:**
- Small heap (256B-1KB)
- Fixed-size pools
- Simple `malloc`/`free`
- No fragmentation mitigation

**High-End:**
- Full heap management
- Slab allocator
- Fragmentation handling
- MPU integration
- Optional: Virtual memory with external RAM

---

## 5. POSIX API Layer

### 5.1 POSIX Profiles

| Profile | Description | Features | Target |
|---------|-------------|----------|--------|
| **PSE51** | Minimal | Single process, no FS | Low-end 8-bit |
| **PSE52** | Multi-threaded | Threads, no FS | Mid-range 8/16-bit |
| **PSE53** | With FS | Threads + FS | Mid/High-end |
| **PSE54** | Full | Multiple processes | High-end 32-bit |

### 5.2 API Coverage

**Low-End (PSE51/52):**
```c
/* lib/posix/unistd/unistd.h */

// Minimal process control
pid_t getpid(void);        // Returns task ID
unsigned sleep(unsigned seconds);  // Task sleep

// Stub functions (return ENOSYS)
pid_t fork(void);          // NOT SUPPORTED
int execve(...);           // NOT SUPPORTED
int pipe(int fd[2]);       // NOT SUPPORTED

/* lib/posix/pthread/pthread.h */

// Basic threading (if RAM allows)
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
void pthread_exit(void *retval);
int pthread_join(pthread_t thread, void **retval);

// Synchronization
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

**Mid-Range (PSE53):**
```c
/* Add file I/O */
int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

/* Add basic networking */
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, ...);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, ...);
```

**High-End (PSE54):**
```c
/* Full POSIX process model */
pid_t fork(void);          // Process creation (if MPU available)
int execve(...);           // Program loading
int waitpid(...);          // Process synchronization

/* Advanced IPC */
int pipe(int fd[2]);
int select(int nfds, fd_set *readfds, fd_set *writefds, ...);

/* Signal handling */
int kill(pid_t pid, int sig);
sighandler_t signal(int signum, sighandler_t handler);
```

### 5.3 Stub Implementation

For unsupported features on low-end devices:

```c
/* lib/posix/stubs/fork_stub.c */

#include <unistd.h>
#include <errno.h>

pid_t fork(void) {
    errno = ENOSYS;  // Function not implemented
    return -1;
}
```

This allows code to compile across all targets, even if functionality is limited.

---

## 6. Driver Architecture

### 6.1 Device Driver Model

```c
/* drivers/common/device.h */

typedef struct device {
    const char *name;
    int (*open)(struct device *dev, int flags);
    int (*close)(struct device *dev);
    ssize_t (*read)(struct device *dev, void *buf, size_t len);
    ssize_t (*write)(struct device *dev, const void *buf, size_t len);
    int (*ioctl)(struct device *dev, unsigned long request, void *arg);
    void *priv;  // Driver-private data
} device_t;
```

### 6.2 Virtual Filesystem (VFS)

**Low-End:**
- Single filesystem type
- No mount table
- Hardcoded paths

**Mid-Range:**
- Multiple FS types
- Simple mount table
- Basic path resolution

**High-End:**
- Full VFS layer
- Multiple mounts
- Symlinks
- Device nodes

### 6.3 Networking Stack

**Low-End:**
- SLIP only
- Raw packets
- No TCP/UDP

**Mid-Range:**
- Minimal TCP/UDP (µIP or custom)
- IPv4 only
- Single socket

**High-End:**
- Full lwIP integration
- TCP/UDP
- IPv4/IPv6
- Multiple sockets
- DNS, DHCP

---

## 7. Build System & Configuration

### 7.1 Profile Configuration

```ini
# config/profiles/low_profile.conf
[profile]
name = "low-end"
description = "8-bit MCUs with minimal resources"
min_ram = 128
max_ram = 4096
min_flash = 4096
max_flash = 32768

[features]
scheduler = "simple"
max_tasks = 4
threading = false
filesystem = false
networking = false
dynamic_memory = false
posix_profile = "PSE51"

[optimizations]
optimization_level = "s"  # -Os
lto = true
size_gate = true
```

```ini
# config/profiles/mid_profile.conf
[profile]
name = "mid-range"
description = "Enhanced 8-bit and 16-bit MCUs"
min_ram = 4096
max_ram = 16384
min_flash = 32768
max_flash = 131072

[features]
scheduler = "preemptive"
max_tasks = 16
threading = true
filesystem = "optional"
networking = "optional"
dynamic_memory = "limited"
posix_profile = "PSE52"

[optimizations]
optimization_level = "s"
lto = true
size_gate = true
```

```ini
# config/profiles/high_profile.conf
[profile]
name = "high-end"
description = "32-bit MCUs with rich resources"
min_ram = 16384
max_ram = 1048576
min_flash = 131072
max_flash = 2097152

[features]
scheduler = "full"
max_tasks = 64
threading = true
filesystem = true
networking = true
dynamic_memory = true
posix_profile = "PSE54"
mpu = "optional"

[optimizations]
optimization_level = "2"  # -O2
lto = true
size_gate = false
```

### 7.2 Package System

```ini
# config/packages/filesystem.conf
[package]
name = "filesystem"
description = "Filesystem support"
requires = ["block_device"]
ram_cost = 512
flash_cost = 2048

[options]
fs_types = ["ramfs", "romfs", "eepfs"]
max_open_files = 8
vfs = true
```

```ini
# config/packages/networking.conf
[package]
name = "networking"
description = "Network stack"
requires = ["timer"]
ram_cost = 8192
flash_cost = 16384

[options]
protocols = ["slip", "ipv4"]
tcp_udp = "optional"
sockets = true
max_sockets = 4
```

### 7.3 Meson Integration

```meson
# config/meson.build

# Load profile
profile = get_option('profile')  # 'low', 'mid', 'high'
profile_conf = configuration_data()
profile_conf.merge_from(files('profiles/' + profile + '_profile.conf'))

# Load packages
packages = get_option('packages')  # ['filesystem', 'networking', ...]
foreach pkg : packages
    pkg_conf = configuration_data()
    pkg_conf.merge_from(files('packages/' + pkg + '.conf'))
    # Validate dependencies, RAM/flash costs
endforeach

# Generate config.h
configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: profile_conf
)
```

---

## 8. ATmega128* Support

### 8.1 ATmega128 Family Overview

| Model | Flash | SRAM | EEPROM | I/O | Notes |
|-------|-------|------|--------|-----|-------|
| **ATmega128** | 128KB | 4KB | 4KB | 53 | Original |
| **ATmega128A** | 128KB | 4KB | 4KB | 53 | Low-power variant |
| **ATmega1280** | 128KB | 8KB | 4KB | 86 | Extended I/O |
| **ATmega1281** | 128KB | 8KB | 4KB | 54 | More timers |
| **ATmega1284** | 128KB | 16KB | 4KB | 32 | Max SRAM |
| **ATmega1284P** | 128KB | 16KB | 4KB | 32 | picoPower |

### 8.2 Profile Assignment

- **ATmega128/128A**: Mid-range profile (4KB SRAM)
- **ATmega1280/1281**: Mid-range+ profile (8KB SRAM)
- **ATmega1284/1284P**: High-end profile (16KB SRAM)

### 8.3 Implementation Plan

1. **HAL Implementation** (`arch/avr8/atmega128/`)
   - Context switch (similar to ATmega328P but 4KB stack space)
   - Timer setup (use Timer1 for 16-bit precision)
   - Interrupt vectors
   - UART drivers (USART0/USART1)

2. **Cross-compilation File** (`cross/atmega128_gcc14.cross`)
   - Already exists, needs validation

3. **Example Application** (`examples/mid_tier/atmega128_demo.c`)
   - Demonstrate threading
   - Filesystem support
   - UART communication

4. **Tests** (`tests/atmega128/`)
   - Scheduler stress test with more tasks
   - Memory allocation test
   - Filesystem performance test

---

## 9. Migration Roadmap

### Phase 1: Foundation (Week 1)
- [ ] Create new directory structure
- [ ] Design HAL interface
- [ ] Create build system infrastructure
- [ ] Write architecture documentation

### Phase 2: AVR8 HAL (Week 2)
- [ ] Extract AVR-specific code to `arch/avr8/`
- [ ] Implement HAL for ATmega328P
- [ ] Implement HAL for ATmega128
- [ ] Test cross-compilation

### Phase 3: Kernel Refactoring (Week 3)
- [ ] Move scheduler to `kernel/sched/`
- [ ] Move IPC to `kernel/ipc/`
- [ ] Move sync primitives to `kernel/sync/`
- [ ] Make kernel portable (use HAL only)

### Phase 4: POSIX Layer (Week 4)
- [ ] Create POSIX API structure
- [ ] Implement PSE51 subset
- [ ] Implement PSE52 subset
- [ ] Create stubs for unsupported functions

### Phase 5: Drivers (Week 5)
- [ ] Refactor filesystem drivers
- [ ] Create VFS layer
- [ ] Refactor UART drivers
- [ ] Refactor network drivers

### Phase 6: Configuration System (Week 6)
- [ ] Create profile configurations
- [ ] Create package system
- [ ] Integrate with Meson
- [ ] Test all combinations

### Phase 7: Testing & Validation (Week 7)
- [ ] Port existing tests
- [ ] Create new tier-specific tests
- [ ] Validate ATmega128 support
- [ ] CI/CD integration

### Phase 8: Documentation (Week 8)
- [ ] Update README
- [ ] Create porting guide
- [ ] API documentation
- [ ] Examples and tutorials

---

## 10. Success Criteria

### 10.1 Functionality

- [ ] Builds for ATmega328P (low-end)
- [ ] Builds for ATmega128/1284P (mid/high-end)
- [ ] Builds for ARM Cortex-M (high-end)
- [ ] All tests pass on all targets
- [ ] POSIX API compiles on all targets
- [ ] Examples run on hardware

### 10.2 Performance

- [ ] Low-end fits in 4KB flash
- [ ] Mid-range fits in 32KB flash
- [ ] No performance regression vs. µ-UNIX baseline
- [ ] Context switch overhead < 200 cycles

### 10.3 Code Quality

- [ ] No warnings with `-Wall -Wextra -Werror`
- [ ] 80%+ code coverage
- [ ] Documentation for all APIs
- [ ] Consistent coding style

---

## 11. References

1. POSIX.1-2008 Standard
2. Apache NuttX Architecture
3. Zephyr RTOS Design
4. FreeRTOS Documentation
5. lwIP Network Stack
6. ATmega128 Datasheet
7. ARM Cortex-M Architecture Reference Manual

---

**Document Status:** Draft
**Last Updated:** 2025-11-19
**Author:** Claude + Avrix Team
