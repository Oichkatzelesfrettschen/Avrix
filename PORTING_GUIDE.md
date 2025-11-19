# Avrix Porting Guide

**Target Audience:** Developers porting Avrix to new microcontroller architectures

**Supported Platforms:** AVR8 (complete), ARM Cortex-M (planned), MSP430 (planned), RISC-V (planned), x86/x64 (host testing)

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Requirements](#architecture-requirements)
3. [HAL Implementation](#hal-implementation)
4. [Step-by-Step Porting Process](#step-by-step-porting-process)
5. [Testing and Validation](#testing-and-validation)
6. [Performance Tuning](#performance-tuning)
7. [Common Pitfalls](#common-pitfalls)

---

## Overview

Avrix uses a **Hardware Abstraction Layer (HAL)** to isolate architecture-specific code. Porting to a new architecture requires implementing the HAL interface for your target.

### Effort Estimate
- **Minimal port** (context switching only): 2-4 hours
- **Complete port** (all HAL functions): 1-2 days
- **Optimized port** (assembly, tuning): 3-5 days

### Files to Implement
```
arch/
├── <your_arch>/
│   ├── common/
│   │   └── hal_<your_arch>.c       # HAL implementation
│   ├── atmega128/                  # MCU-specific (optional)
│   │   ├── hal_atmega128.c
│   │   └── meson.build
│   └── meson.build
└── common/
    └── hal.h                       # HAL interface (DO NOT MODIFY)
```

---

## Architecture Requirements

### Minimum Requirements (PSE51)
- **CPU**: 8-bit or better
- **Flash**: 8 KB minimum (16+ KB recommended)
- **RAM**: 1 KB minimum (2+ KB recommended)
- **Stack**: Separate stack pointer (or emulated)
- **Interrupts**: Basic interrupt support

### PSE52 Requirements (Multi-Threading)
- **Flash**: 32 KB minimum
- **RAM**: 4 KB minimum
- **Timer**: 1 kHz tick timer for scheduler
- **Atomics**: Atomic operations (or interrupt disable fallback)

### PSE54 Requirements (Full POSIX)
- **MMU**: Memory Management Unit
- **Flash**: 128 KB minimum
- **RAM**: 16 MB minimum
- **Processes**: fork/exec support

---

## HAL Implementation

The HAL is defined in `arch/common/hal.h`. You must implement all functions for your architecture.

### HAL Categories

#### 1. Context Switching (REQUIRED)
```c
void hal_context_init(hal_context_t *ctx, void *stack_top,
                      void (*entry)(void), void *arg);
void hal_context_switch(hal_context_t **old_ctx, hal_context_t *new_ctx);
```

**Implementation Notes:**
- Save/restore all registers (including PC, SP, status register)
- ARM: Use `push {r4-r11,lr}` + `pop {r4-r11,pc}`
- AVR: Save 32 registers + SREG + PC
- RISC-V: Save ra, sp, s0-s11, caller-saved if needed

**Example (ARM Cortex-M0):**
```c
void hal_context_switch(hal_context_t **old_ctx, hal_context_t *new_ctx) {
    asm volatile (
        "push {r4-r7,lr}\n"          // Save registers
        "mov  r4, r8\n"
        "mov  r5, r9\n"
        "mov  r6, r10\n"
        "mov  r7, r11\n"
        "push {r4-r7}\n"
        "str  sp, [r0]\n"            // Save SP to old_ctx
        "ldr  sp, [r1]\n"            // Load SP from new_ctx
        "pop  {r4-r7}\n"
        "mov  r8, r4\n"
        "mov  r9, r5\n"
        "mov  r10, r6\n"
        "mov  r11, r7\n"
        "pop  {r4-r7,pc}\n"          // Restore and return
    );
}
```

#### 2. Atomic Operations (REQUIRED for PSE52)
```c
uint8_t hal_atomic_compare_and_swap_8(volatile uint8_t *ptr,
                                       uint8_t expected, uint8_t desired);
void hal_atomic_store_8(volatile uint8_t *ptr, uint8_t val);
uint8_t hal_atomic_load_8(const volatile uint8_t *ptr);
```

**Implementation Options:**
- **Hardware atomics**: ARM LDREX/STREX, RISC-V A extension
- **Interrupt disable**: AVR cli/sei
- **Test-and-set**: x86 LOCK CMPXCHG

**Example (AVR with interrupt disable):**
```c
uint8_t hal_atomic_compare_and_swap_8(volatile uint8_t *ptr,
                                       uint8_t expected, uint8_t desired) {
    uint8_t sreg = SREG;
    cli();  // Disable interrupts
    uint8_t current = *ptr;
    if (current == expected) {
        *ptr = desired;
    }
    SREG = sreg;  // Restore interrupts
    return current;
}
```

#### 3. Memory Barriers (REQUIRED for multi-core)
```c
void hal_memory_barrier(void);
```

**Implementation:**
- **ARM**: `dmb`, `dsb`, `isb` instructions
- **x86**: `mfence`, `lfence`, `sfence`
- **Single-core**: Can be no-op if no out-of-order execution

#### 4. PROGMEM Support (OPTIONAL)
```c
uint8_t hal_pgm_read_byte(const void *addr);
void hal_memcpy_P(void *dest, const void *src, size_t n);
```

**Implementation:**
- AVR: Use `pgm_read_byte()` from `<avr/pgmspace.h>`
- ARM: Identity functions (flash is memory-mapped)
- x86: Identity functions

#### 5. EEPROM Support (OPTIONAL)
```c
bool hal_eeprom_available(void);
uint8_t hal_eeprom_read_byte(uint16_t addr);
void hal_eeprom_update_byte(uint16_t addr, uint8_t val);
```

**Implementation:**
- AVR: Use `<avr/eeprom.h>`
- ARM: Emulate with flash sectors (wear-leveling!)
- x86: Emulate with file I/O

---

## Step-by-Step Porting Process

### Step 1: Create Directory Structure
```bash
mkdir -p arch/your_arch/common
cp arch/avr8/common/hal_avr8.c arch/your_arch/common/hal_your_arch.c
```

### Step 2: Implement Context Switching
Start with a minimal implementation:
```c
// hal_your_arch.c
#include "arch/common/hal.h"

void hal_context_init(hal_context_t *ctx, void *stack_top,
                      void (*entry)(void), void *arg) {
    // TODO: Initialize stack frame
    // 1. Set stack pointer
    // 2. Push entry function address
    // 3. Push initial register values
}

void hal_context_switch(hal_context_t **old_ctx, hal_context_t *new_ctx) {
    // TODO: Save current context
    // TODO: Restore new context
}
```

### Step 3: Test Context Switching
```c
// Test program
void task1(void) {
    while (1) {
        printf("Task 1\n");
        // Yield
    }
}

void task2(void) {
    while (1) {
        printf("Task 2\n");
        // Yield
    }
}

int main(void) {
    hal_context_t ctx1, ctx2;
    uint8_t stack1[256], stack2[256];

    hal_context_init(&ctx1, stack1 + 256, task1, NULL);
    hal_context_init(&ctx2, stack2 + 256, task2, NULL);

    // Test switch
    hal_context_switch(&ctx1, &ctx2);
    // Should print "Task 2"
}
```

### Step 4: Implement Atomics
```c
uint8_t hal_atomic_compare_and_swap_8(volatile uint8_t *ptr,
                                       uint8_t expected, uint8_t desired) {
    // Start with interrupt-disable approach
    // Optimize later with hardware atomics
}
```

### Step 5: Implement Memory Barriers
```c
void hal_memory_barrier(void) {
    // ARM: __asm__ volatile ("dmb" ::: "memory");
    // x86: __asm__ volatile ("mfence" ::: "memory");
    // Single-core: __asm__ volatile ("" ::: "memory");  // Compiler barrier
}
```

### Step 6: Create Meson Build File
```meson
# arch/your_arch/meson.build
your_arch_sources = files(
  'common/hal_your_arch.c',
)

your_arch_inc = include_directories('common')

your_arch_dep = declare_dependency(
  sources             : your_arch_sources,
  include_directories : your_arch_inc,
)
```

### Step 7: Update Root Meson
```meson
# meson.build (add to architecture selection)
if host_machine.cpu_family() == 'your_arch'
  subdir('arch/your_arch')
  arch_dep = your_arch_dep
endif
```

---

## Testing and Validation

### Test Suite
1. **Context Switch Test**: Verify task switching works
2. **Atomic Test**: Verify CAS operations
3. **Stress Test**: Switch rapidly between 10+ tasks
4. **Stack Test**: Verify no stack corruption
5. **Interrupt Test**: Verify atomics work during interrupts

### Validation Commands
```bash
meson setup build --cross-file cross/your_arch.cross
meson compile -C build
meson test -C build
```

### Debug Tips
- **Stack corruption**: Check alignment requirements
- **Random crashes**: Verify all registers saved/restored
- **Atomic failures**: Use interrupt disable as fallback
- **Performance**: Profile with timer, optimize hot paths

---

## Performance Tuning

### Context Switch Optimization
**Target: <50 cycles**

1. **Use assembly**: Hand-optimize register save/restore
2. **Minimize saves**: Only save callee-saved registers
3. **Lazy FPU**: Don't save FPU registers unless used
4. **Hardware support**: Use CPU-specific instructions (e.g., ARM push/pop multiple)

**Example (optimized AVR8):**
```asm
; 20-cycle context switch (AVR8 @ 16 MHz = 1.25 µs)
hal_context_switch:
    ; Save context (10 cycles)
    push r2   ; 32 registers
    push r3
    ; ... (30 more pushes)
    push r29
    in   r0, 0x3f  ; SREG
    push r0

    ; Switch stack (6 cycles)
    ld   r26, X+
    ld   r27, X
    out  0x3d, r26  ; SPL
    out  0x3e, r27  ; SPH

    ; Restore context (10 cycles)
    pop  r0
    out  0x3f, r0  ; SREG
    pop  r29
    ; ... (30 more pops)
    pop  r2
    ret
```

### Atomic Operations Optimization
**Target: <10 cycles**

- ARM: Use LDREX/STREX (2-4 cycles)
- x86: Use LOCK CMPXCHG (10-20 cycles)
- AVR: cli/sei (2 cycles each)

---

## Common Pitfalls

### 1. Incorrect Stack Alignment
**Problem:** ARM requires 8-byte alignment, crashes if violated

**Solution:**
```c
#define STACK_ALIGN 8
void *stack_top = (void *)((uintptr_t)(stack + size) & ~(STACK_ALIGN - 1));
```

### 2. Forgetting Status Register
**Problem:** Arithmetic flags corrupted after context switch

**Solution:** Always save/restore CPU status register (SREG, CPSR, FLAGS)

### 3. Interrupt-Unsafe Atomics
**Problem:** Atomics don't work in ISR context

**Solution:** Use interrupt-disable for critical sections in ISR

### 4. Endianness Issues
**Problem:** Multi-byte values corrupted (network protocols)

**Solution:** Use `hal_swap_bytes()` or define `HAL_BIG_ENDIAN`

### 5. Insufficient Stack Size
**Problem:** Random crashes due to stack overflow

**Solution:** Allocate 256+ bytes per task, use stack canaries

---

## Architecture-Specific Notes

### ARM Cortex-M
- **Context**: Use PendSV for context switch
- **Atomics**: LDREX/STREX (M3+), disable interrupts (M0)
- **Stack**: Descending, 8-byte aligned
- **Quirks**: Hardware stacks 8 registers on exception

### AVR8 (ATmega)
- **Context**: 32 registers + SREG + PC (35 bytes)
- **Atomics**: cli/sei (interrupt disable)
- **Stack**: Ascending
- **Quirks**: 2-byte PC on >64KB devices

### RISC-V
- **Context**: 32 registers (RV32I) or 16 (RV32E)
- **Atomics**: Use A extension (amoadd, amoswap)
- **Stack**: Descending, 16-byte aligned
- **Quirks**: No status register (use CSRs)

### MSP430
- **Context**: 16 registers + SR
- **Atomics**: Disable GIE bit
- **Stack**: Descending
- **Quirks**: 16-bit architecture

---

## Example: Complete ARM Cortex-M Port

See `arch/armcm/common/hal_armcm.c` (when available) for a complete reference implementation.

---

## Support

- **GitHub Issues**: https://github.com/anthropics/avrix/issues
- **Documentation**: `docs/` directory
- **Examples**: `arch/avr8/` (reference implementation)

---

*Last Updated: Phase 8, Session 2025-01-XX*
