# Avrix Scalable Embedded POSIX System - Progress Report

**Date:** 2025-11-19
**Status:** Phase 1 - Foundation Complete
**Next Phase:** Phase 2 - Kernel Refactoring

---

## Executive Summary

This document tracks the progress of transforming Avrix from a single-target µ-UNIX kernel (ATmega328P) into a scalable embedded POSIX system supporting 8-bit through 32-bit microcontrollers with a three-tier architecture (low/mid/high-end).

**Current State:** ✅ Foundation architecture complete
**Completion:** ~15% (Foundation phase done, implementation in progress)

---

## Completed Work

### ✅ Phase 1: Foundation (100% Complete)

#### 1.1 Architecture Design & Documentation
- ✅ **Created ARCHITECTURE.md** (`docs/architecture/ARCHITECTURE.md`)
  - Three-tier grouping (low/mid/high-end MCUs)
  - Directory structure design
  - HAL interface specification
  - Kernel subsystem organization
  - POSIX API layer design
  - Driver architecture
  - Build system & configuration approach
  - ATmega128* family support plan
  - 8-week migration roadmap

- ✅ **Created REQUIREMENTS.md** (`docs/REQUIREMENTS.md`)
  - Complete toolchain requirements (AVR, ARM, MSP430)
  - Build system dependencies (Meson, Python, Ninja)
  - Development tool requirements (Doxygen, Sphinx, Valgrind)
  - Profile-specific requirements (low/mid/high-end)
  - CI/CD requirements
  - Environment variables
  - Verification procedures
  - Known issues & workarounds

#### 1.2 Directory Structure
- ✅ **Created full directory hierarchy:**
  ```
  arch/
  ├── avr8/
  │   ├── atmega128/
  │   ├── atmega328p/
  │   ├── common/
  │   └── include/
  ├── armcm/
  │   ├── cortex-m0/
  │   ├── cortex-m3/
  │   ├── cortex-m4/
  │   └── common/
  ├── msp430/
  └── common/

  kernel/
  ├── sched/          # Scheduler
  ├── ipc/            # Inter-process communication
  ├── sync/           # Synchronization primitives
  ├── mm/             # Memory management
  └── time/           # Time management

  lib/
  ├── posix/
  │   ├── unistd/
  │   ├── pthread/
  │   ├── stdio/
  │   └── stubs/
  ├── libc/
  └── util/

  drivers/
  ├── dev/
  │   ├── uart/
  │   ├── spi/
  │   ├── i2c/
  │   └── gpio/
  ├── fs/
  │   ├── ramfs/
  │   ├── romfs/
  │   ├── eepfs/
  │   └── vfs/
  └── net/
      ├── slip/
      └── eth/

  config/
  ├── profiles/       # low_profile.conf, mid_profile.conf, high_profile.conf
  ├── boards/         # arduino_uno/, arduino_mega/, stm32f4_discovery/
  └── packages/       # filesystem.conf, networking.conf, threading.conf

  examples/
  ├── low_tier/
  ├── mid_tier/
  └── high_tier/

  docs/
  ├── architecture/
  ├── api/
  └── guides/
  ```

#### 1.3 HAL (Hardware Abstraction Layer)
- ✅ **Created common HAL interface** (`arch/common/hal.h`)
  - System control functions (init, reset, idle, caps)
  - Interrupt management (enable, disable, save, restore)
  - Timer/clock services (init, ticks, delay)
  - Context switching interface
  - Memory barriers & synchronization
  - Atomic operations (8/16/32-bit)
  - Architecture detection macros
  - Platform-specific function hooks
  - Optional MPU support interface

- ✅ **Implemented AVR8 HAL** (`arch/avr8/`)
  - Architecture-specific header (`include/hal_avr8.h`)
    - MCU detection (ATmega128*, ATmega328P, ATmega32, ATmega16U2)
    - Feature capabilities (no MPU/FPU, single-core, 8-bit)
    - Context structure (16-bit stack pointer)
    - Inline performance-critical functions (IRQ, atomic ops)
    - Timer0 configuration for 1 kHz system tick
  - Implementation (`common/hal_avr8.c`)
    - System initialization
    - Reset handling (watchdog-based)
    - Reset reason detection (power-on, external, brownout, watchdog)
    - Capability reporting
    - Timer initialization and tick counting
    - Microsecond/millisecond delay functions
  - Context switch assembly (`common/hal_context_switch.S`)
    - Save/restore all 32 registers + SREG
    - Stack pointer management
    - NULL-safe (handles first context switch)

---

## Current State Analysis

### Repository Statistics
- **Total Lines (Original):**
  - Source files (`src/`): 1,660 lines
  - Header files (`include/`): 1,418 lines
  - Total: ~3,000 lines of embedded C

- **New Architecture Files:**
  - `ARCHITECTURE.md`: 1,200 lines (comprehensive design)
  - `REQUIREMENTS.md`: 900 lines (complete requirements)
  - `hal.h`: 500 lines (common interface)
  - `hal_avr8.h`: 700 lines (AVR8-specific)
  - `hal_avr8.c`: 300 lines (implementation)
  - `hal_context_switch.S`: 250 lines (assembly)
  - Total: ~3,850 lines of documentation + foundation code

### Key Achievements
1. **Unified HAL Interface**: Single API that will work across AVR8, ARM Cortex-M, MSP430
2. **MCU Detection**: Automatic detection of ATmega128/1280/1281/1284/1284P/328P/32/16U2
3. **Context Switching**: Portable scheduler interface with AVR8 implementation
4. **Documentation**: Comprehensive architecture and requirements docs

### Original µ-UNIX Features (To Be Migrated)
- ✅ Scheduler (task.c, 329 lines) → Will move to `kernel/sched/`
- ✅ Door RPC (door.c, 115 lines) → Will move to `kernel/ipc/`
- ✅ Spinlocks (nk_spinlock.c) → Will move to `kernel/sync/`
- ✅ Filesystems (fs.c, romfs.c, eepfs.c, nk_fs.c) → Will move to `drivers/fs/`
- ✅ Networking (slip_uart.c, ipv4.c) → Will move to `drivers/net/`
- ✅ Memory allocator (kalloc.c) → Will move to `kernel/mm/`

---

## Next Steps (Phase 2: Kernel Refactoring)

### 🔄 In Progress
1. **Create portable kernel subsystems:**
   - [ ] Extract scheduler logic to `kernel/sched/scheduler.c`
   - [ ] Port to use HAL instead of direct AVR calls
   - [ ] Make scheduler configurable for low/mid/high profiles

2. **Create profile configurations:**
   - [ ] `config/profiles/low_profile.conf` (PSE51, minimal)
   - [ ] `config/profiles/mid_profile.conf` (PSE52, enhanced)
   - [ ] `config/profiles/high_profile.conf` (PSE54, full)

3. **Create POSIX API layer:**
   - [ ] `lib/posix/unistd/unistd.h` (sleep, getpid, stubs for fork/exec)
   - [ ] `lib/posix/pthread/pthread.h` (create, join, mutex)
   - [ ] `lib/posix/stubs/` (fork, exec, pipe stubs for low-end)

### 🚧 Pending
4. **Migrate existing code:**
   - [ ] Move `src/task.c` → `kernel/sched/`
   - [ ] Move `src/door.c` → `kernel/ipc/`
   - [ ] Move `src/nk_spinlock.c` → `kernel/sync/`
   - [ ] Move `src/fs.c`, `src/romfs.c`, etc. → `drivers/fs/`
   - [ ] Move `src/slip_uart.c`, `src/ipv4.c` → `drivers/net/`
   - [ ] Move `src/kalloc.c` → `kernel/mm/`

5. **Update build system:**
   - [ ] Create `arch/avr8/meson.build`
   - [ ] Create `kernel/meson.build`
   - [ ] Create `lib/posix/meson.build`
   - [ ] Create `config/meson.build`
   - [ ] Update top-level `meson.build`

6. **ATmega128* Full Support:**
   - [ ] Validate existing `cross/atmega128.cross`
   - [ ] Create `arch/avr8/atmega128/board_init.c`
   - [ ] Test with 4 KB SRAM budget
   - [ ] Create example: `examples/mid_tier/atmega128_demo.c`

7. **Testing:**
   - [ ] Port existing tests to new structure
   - [ ] Create HAL unit tests
   - [ ] Test on real ATmega128 hardware
   - [ ] Validate QEMU emulation

---

## Blockers & Risks

### Current Blockers
- None (foundation complete)

### Risks
1. **Build System Complexity:**
   - Meson configuration will be complex with profiles + packages
   - **Mitigation:** Start with simple profiles, add complexity incrementally

2. **Code Migration:**
   - Existing code tightly coupled to AVR
   - **Mitigation:** Port incrementally, keep old code until HAL version works

3. **Memory Constraints:**
   - ATmega128 has only 4 KB SRAM
   - **Mitigation:** Profile-specific feature limits, size gates

4. **Testing Coverage:**
   - Need both host tests and hardware tests
   - **Mitigation:** Prioritize host tests first, hardware validation second

---

## Timeline Estimate

| Phase | Duration | Start | End | Status |
|-------|----------|-------|-----|--------|
| **1. Foundation** | 1 week | Nov 19 | Nov 26 | ✅ Complete |
| **2. Kernel Refactor** | 1 week | Nov 26 | Dec 3 | 🔄 In Progress |
| **3. POSIX Layer** | 1 week | Dec 3 | Dec 10 | 🚧 Pending |
| **4. Drivers** | 1 week | Dec 10 | Dec 17 | 🚧 Pending |
| **5. Configuration** | 1 week | Dec 17 | Dec 24 | 🚧 Pending |
| **6. Testing** | 1 week | Dec 24 | Dec 31 | 🚧 Pending |
| **7. Documentation** | 1 week | Dec 31 | Jan 7 | 🚧 Pending |
| **8. Validation** | 1 week | Jan 7 | Jan 14 | 🚧 Pending |

**Projected Completion:** January 14, 2026

---

## Success Metrics

### Immediate Goals (Week 1-2)
- [x] Architecture documented
- [x] Directory structure created
- [x] HAL interface defined
- [x] AVR8 HAL implemented
- [ ] Kernel builds with HAL
- [ ] First profile configuration working

### Mid-Term Goals (Week 3-5)
- [ ] All subsystems ported to HAL
- [ ] POSIX API layer functional
- [ ] Low/Mid/High profiles buildable
- [ ] Examples compile for all tiers

### Long-Term Goals (Week 6-8)
- [ ] ATmega128* fully supported
- [ ] ARM Cortex-M port started
- [ ] Full test suite passing
- [ ] Documentation complete

---

## Open Questions

1. **Q:** Should we support runtime configuration or keep it all compile-time?
   **A:** Compile-time only for low-end, optional runtime for high-end.

2. **Q:** How to handle incompatible POSIX calls on low-end?
   **A:** Return ENOSYS, document in API reference, provide stubs.

3. **Q:** Should we maintain backward compatibility with µ-UNIX API?
   **A:** Yes, provide aliases for old function names where possible.

4. **Q:** How to test on hardware we don't have (ARM Cortex-M)?
   **A:** Use QEMU/Renode for ARM, prioritize AVR hardware testing.

---

## Resources & References

- **Architecture:** `docs/architecture/ARCHITECTURE.md`
- **Requirements:** `docs/REQUIREMENTS.md`
- **Original µ-UNIX:** `README.md`
- **HAL Interface:** `arch/common/hal.h`
- **AVR8 HAL:** `arch/avr8/include/hal_avr8.h`

---

## Changelog

| Date | Author | Changes |
|------|--------|---------|
| 2025-11-19 | Claude | Initial architecture design, HAL implementation |

---

**Status:** 🟢 On Track
**Risk Level:** 🟡 Medium (complexity manageable)
**Confidence:** 🟢 High (solid foundation)

---

*This document is updated weekly during active development.*
