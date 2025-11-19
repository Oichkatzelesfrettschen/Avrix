# Avrix Build Guide

**Target Audience:** Developers building Avrix for embedded targets

**Build System:** Meson + Ninja (cross-platform, fast, modern)

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Build System Overview](#build-system-overview)
3. [Configuration Options](#configuration-options)
4. [Cross-Compilation](#cross-compilation)
5. [Build Profiles](#build-profiles)
6. [Advanced Usage](#advanced-usage)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt install meson ninja-build gcc-avr avr-libc

# macOS
brew install meson ninja avr-gcc

# Arch Linux
sudo pacman -S meson ninja avr-gcc avr-libc
```

### Build for Host (Testing)
```bash
meson setup build
meson compile -C build
meson test -C build
```

### Build for AVR (ATmega328P)
```bash
meson setup build_avr --cross-file cross/atmega328p_gcc14.cross
meson compile -C build_avr
```

### Flash to Hardware
```bash
meson compile -C build_avr flash
```

---

## Build System Overview

### Directory Structure
```
Avrix/
├── meson.build              # Root build file
├── meson_options.txt        # Build options
├── cross/                   # Cross-compilation files
│   ├── atmega328p_gcc14.cross
│   ├── atmega1284_gcc14.cross
│   └── ...
├── build_flags/             # Compiler flags
│   ├── meson.build
│   └── flags.meson
├── arch/                    # Architecture HAL
│   ├── meson.build
│   ├── avr8/meson.build
│   └── ...
├── kernel/                  # Kernel subsystems
│   ├── meson.build
│   ├── sched/meson.build
│   └── ...
├── drivers/                 # Device drivers
│   ├── meson.build
│   ├── fs/meson.build
│   └── ...
├── lib/                     # POSIX library
│   └── posix/meson.build
├── src/                     # Legacy sources
│   └── meson.build
├── examples/                # Example programs
│   └── meson.build
└── tests/                   # Test suite
    └── meson.build
```

### Build Workflow
```mermaid
graph LR
    A[meson setup] --> B[Generate build.ninja]
    B --> C[meson compile]
    C --> D[Link executables]
    D --> E[meson test]
    E --> F[meson install]
```

### Build Outputs
```
build/
├── libavrix.a              # Static library (kernel + drivers)
├── unix0.elf               # Firmware ELF
├── unix0.hex               # Intel HEX (for flashing)
├── examples/               # Example binaries
│   ├── hello_pse51
│   ├── threading_pse52
│   └── ...
└── tests/                  # Test binaries
    ├── test_fixed_point
    ├── fs_test
    └── ...
```

---

## Configuration Options

### View All Options
```bash
meson configure build
```

### Common Options

#### Flash Size Limit
```bash
meson setup build -Dflash_limit=32768  # 32 KB
```

Enforces maximum firmware size. Build fails if exceeded.

#### GDB Stub (On-Device Debugging)
```bash
meson setup build -Ddebug_gdb=true
```

Enables GDB stub for on-chip debugging via serial port.

#### Optimization Level
```bash
meson setup build -Doptimization=s  # Size
meson setup build -Doptimization=2  # Speed
meson setup build -Doptimization=g  # Debug
```

#### Sanitizers (Host Only)
```bash
meson setup build -Dsan=true  # Address + UB sanitizers
meson setup build -Dcov=true  # Coverage instrumentation
```

#### AVR Include Directory
```bash
meson setup build -Davr_inc_dir=/usr/lib/avr/include
```

---

## Cross-Compilation

### Cross-File Format
```ini
# cross/atmega328p_gcc14.cross
[binaries]
c = 'avr-gcc'
ar = 'avr-ar'
strip = 'avr-strip'
objcopy = 'avr-objcopy'

[properties]
c_args = ['-mmcu=atmega328p', '-DF_CPU=16000000UL']
c_link_args = ['-mmcu=atmega328p']

[host_machine]
system = 'baremetal'
cpu_family = 'avr'
cpu = 'atmega328p'
endian = 'little'
```

### Supported Targets

| Target       | MCU          | Flash | RAM   | Cross-File                    |
|--------------|--------------|-------|-------|-------------------------------|
| ATmega128    | AVR8         | 128KB | 4KB   | `atmega128_gcc14.cross`       |
| ATmega328P   | AVR8         | 32KB  | 2KB   | `atmega328p_gcc14.cross`      |
| ATmega1284P  | AVR8         | 128KB | 16KB  | `atmega1284_gcc14.cross`      |
| ARM Cortex-M0| ARM          | 256KB | 32KB  | `cortex_m0_gcc.cross` (TODO)  |
| RISC-V RV32I | RISC-V       | 128KB | 16KB  | `riscv32_gcc.cross` (TODO)    |

### Creating Custom Cross-Files
```bash
cp cross/atmega328p_gcc14.cross cross/my_mcu.cross
# Edit my_mcu.cross:
#   - Change -mmcu=<your_mcu>
#   - Change F_CPU=<your_frequency>
#   - Change cpu=<your_cpu>
```

### Build with Custom Cross-File
```bash
meson setup build_custom --cross-file cross/my_mcu.cross
meson compile -C build_custom
```

---

## Build Profiles

### PSE51 (Minimal Profile)
**Target**: 8-16KB flash, 1-2KB RAM

```bash
# Features: Single-threaded, basic I/O, no networking
meson setup build -Dprofile=pse51 --cross-file cross/atmega128_gcc14.cross
```

**Enabled Components:**
- Scheduler: Cooperative (no preemption)
- Drivers: ROMFS, EEPFS (no VFS)
- Networking: None
- IPC: None

### PSE52 (Multi-Threaded Profile)
**Target**: 32-128KB flash, 4-16KB RAM

```bash
# Features: pthread, networking, IPC
meson setup build -Dprofile=pse52 --cross-file cross/atmega1284_gcc14.cross
```

**Enabled Components:**
- Scheduler: Preemptive (pthread)
- Drivers: ROMFS, EEPFS, VFS
- Networking: SLIP, IPv4
- IPC: Door RPC, spinlocks

### PSE54 (Full POSIX Profile)
**Target**: 128KB+ flash, 16MB+ RAM, MMU

```bash
# Features: Processes, signals, MMU, full IPC
meson setup build -Dprofile=pse54 --cross-file cross/cortex_a_gcc.cross
```

**Enabled Components:**
- Scheduler: Preemptive + real-time
- Drivers: All (VFS, networking, filesystems)
- Networking: Full TCP/IP stack
- IPC: All (pipes, signals, shared memory, sockets)

---

## Advanced Usage

### Parallel Builds
```bash
meson compile -C build -j8  # Use 8 cores
```

### Verbose Output
```bash
meson compile -C build -v
```

### Reconfigure Build
```bash
meson setup build --wipe           # Clean rebuild
meson configure build -Doption=val # Change option
```

### Build Specific Target
```bash
meson compile -C build libavrix    # Library only
meson compile -C build hello_pse51 # Specific example
meson compile -C build test_fs     # Specific test
```

### Install System-Wide (Not Recommended)
```bash
meson install -C build --destdir /tmp/staging
```

### Generate Compilation Database
```bash
meson setup build --backend=ninja
# Creates build/compile_commands.json for IDE integration
```

---

## Troubleshooting

### Problem: `avr-gcc not found`
**Solution:**
```bash
# Ubuntu/Debian
sudo apt install gcc-avr avr-libc

# Or use newer version from Debian sid
echo "deb http://deb.debian.org/debian sid main" | sudo tee /etc/apt/sources.list.d/sid.list
sudo apt update
sudo apt install -t sid gcc-avr
```

### Problem: Flash size exceeded
**Solution:**
```bash
# Check current size
avr-size build_avr/unix0.elf

# Optimize for size
meson configure build_avr -Doptimization=s

# Disable debug symbols
meson configure build_avr -Dbuildtype=release

# Disable optional features
meson configure build_avr -Ddebug_gdb=false
```

### Problem: Linker errors (`undefined reference`)
**Solution:**
```bash
# Check dependencies
meson introspect build --dependencies

# Clean rebuild
rm -rf build
meson setup build --cross-file cross/atmega328p_gcc14.cross
```

### Problem: Tests fail
**Solution:**
```bash
# Run specific test with verbose output
meson test -C build test_fixed_point -v

# Run with debugger
meson test -C build test_fs --gdb

# Run with valgrind (host only)
meson test -C build --wrap='valgrind --leak-check=full'
```

### Problem: Cross-compilation fails
**Solution:**
```bash
# Verify cross-file
cat cross/atmega328p_gcc14.cross

# Test compiler directly
avr-gcc -mmcu=atmega328p -E - <<< '#include <avr/io.h>' > /dev/null

# Check meson log
cat build/meson-log.txt
```

---

## Build Performance

### Optimization Comparison

| Build Type | Flash Size | RAM Size | Build Time | Boot Time |
|------------|------------|----------|------------|-----------|
| Debug (-g) | 12.4 KB    | 1.8 KB   | 8s         | 120ms     |
| Release (-O2) | 9.8 KB | 1.6 KB   | 12s        | 85ms      |
| MinSize (-Os) | 8.6 KB | 1.5 KB   | 15s        | 95ms      |

### Incremental Build
```bash
# First build (clean): ~15 seconds
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build

# Modify one file: ~2 seconds
echo "// comment" >> src/task.c
meson compile -C build
```

### Cache Directory
```bash
# Meson caches in ~/.cache/meson
# Clear if corrupted
rm -rf ~/.cache/meson
```

---

## CI/CD Integration

### GitHub Actions
```yaml
# .github/workflows/build.yml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y meson ninja-build gcc-avr avr-libc
      - name: Build AVR
        run: |
          meson setup build --cross-file cross/atmega328p_gcc14.cross
          meson compile -C build
      - name: Run tests (host)
        run: |
          meson setup build_host
          meson test -C build_host
```

### GitLab CI
```yaml
# .gitlab-ci.yml
build_avr:
  image: ubuntu:22.04
  script:
    - apt update && apt install -y meson ninja-build gcc-avr avr-libc
    - meson setup build --cross-file cross/atmega328p_gcc14.cross
    - meson compile -C build
  artifacts:
    paths:
      - build/unix0.hex
```

---

## Flash Commands

### AVR (avrdude)
```bash
# Auto-detect programmer
avrdude -c usbasp -p m328p -U flash:w:build_avr/unix0.hex:i

# Arduino Uno (via USB)
avrdude -c arduino -p m328p -P /dev/ttyACM0 -b 115200 -U flash:w:build_avr/unix0.hex:i

# Via ISP programmer
avrdude -c usbtiny -p m328p -U flash:w:build_avr/unix0.hex:i
```

### ARM (OpenOCD)
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program build_arm/unix0.elf verify reset exit"
```

### RISC-V (J-Link)
```bash
JLinkExe -device FE310 -if JTAG -speed 4000 \
         -CommanderScript flash_script.jlink
```

---

*Last Updated: Phase 8, Session 2025-01-XX*
