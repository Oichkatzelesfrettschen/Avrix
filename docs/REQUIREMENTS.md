# Avrix Scalable Embedded POSIX System - Requirements

## Overview

This document specifies all build dependencies, toolchain requirements, and configuration options for the Avrix embedded POSIX system across low-end, mid-range, and high-end microcontroller targets.

**Last Updated:** 2025-11-19
**Version:** 2.0

---

## 1. Build System Requirements

### 1.1 Core Build Tools

| Tool | Minimum Version | Recommended | Purpose |
|------|----------------|-------------|---------|
| **Meson** | 0.56.0 | 1.2.0+ | Build system |
| **Python** | 3.7 | 3.10+ | Meson runner, scripts |
| **Ninja** | 1.10 | 1.11+ | Build backend |
| **Git** | 2.25 | 2.40+ | Version control |
| **Make** | 4.2 | 4.3+ | Optional (legacy) |

**Installation:**

```bash
# Debian/Ubuntu
sudo apt install -y meson ninja-build python3 git

# Fedora/RHEL
sudo dnf install -y meson ninja-build python3 git

# Arch Linux
sudo pacman -S meson ninja python git

# macOS
brew install meson ninja python git
```

---

## 2. Toolchain Requirements

### 2.1 AVR 8-bit Toolchain (Low/Mid-Tier)

**Primary Targets:** ATmega128*, ATmega328P, ATmega32, ATmega16U2

| Component | Minimum | Recommended | Notes |
|-----------|---------|-------------|-------|
| **avr-gcc** | 7.3.0 | 14.2.0 | C23 support in 14+ |
| **avr-binutils** | 2.26 | 2.40+ | Linker, assembler |
| **avr-libc** | 2.0.0 | 2.1.0+ | Standard library |
| **avrdude** | 6.3 | 7.1+ | Firmware flashing |
| **QEMU** | 8.0 | 8.2+ | `arduino-uno` machine |

**Modern Toolchain (C23 support):**

```bash
# Debian Sid (GCC 14)
echo "deb http://deb.debian.org/debian sid main" | \
  sudo tee /etc/apt/sources.list.d/debian-sid.list
sudo apt update
sudo apt install -y -t sid gcc-avr avr-libc binutils-avr

# Verify
avr-gcc --version  # Should show 14.x
```

**Legacy Toolchain (GCC 7.3 - Ubuntu Universe):**

```bash
# Ubuntu 20.04/22.04
sudo apt install -y gcc-avr avr-libc binutils-avr avrdude

# Verify
avr-gcc --version  # Should show 7.3.0 or 5.4.0
```

**Alternative: xPack AVR GCC**

```bash
# Download prebuilt binaries
wget https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
tar xzf xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
export PATH="$PWD/xpack-avr-gcc-13.2.0-1/bin:$PATH"
```

**QEMU for AVR:**

```bash
# Build from source (for `arduino-uno` machine support)
git clone https://gitlab.com/qemu-project/qemu.git
cd qemu
git checkout v8.2.0
./configure --target-list=avr-softmmu
make -j$(nproc)
sudo make install

# Verify
qemu-system-avr -M help | grep arduino-uno
```

### 2.2 ARM Cortex-M Toolchain (High-Tier)

**Primary Targets:** STM32F4, STM32H7, NRF52, SAMD21

| Component | Minimum | Recommended | Notes |
|-----------|---------|-------------|-------|
| **arm-none-eabi-gcc** | 10.2.1 | 12.3+ | ARM GCC |
| **arm-none-eabi-binutils** | 2.35 | 2.40+ | Linker, assembler |
| **arm-none-eabi-newlib** | 3.3.0 | 4.3.0+ | C library |
| **openocd** | 0.11.0 | 0.12.0+ | Debugging/flashing |
| **gdb-multiarch** | 10.1 | 13.1+ | Debugging |

**Installation:**

```bash
# Debian/Ubuntu
sudo apt install -y gcc-arm-none-eabi libnewlib-arm-none-eabi \
  binutils-arm-none-eabi openocd gdb-multiarch

# Verify
arm-none-eabi-gcc --version
```

### 2.3 MSP430 Toolchain (Mid-Tier)

**Primary Targets:** MSP430F5529, MSP430FR5994

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **msp430-gcc** | 9.2.0 | 9.3.1+ |
| **msp430-binutils** | 2.34 | 2.40+ |
| **mspdebug** | 0.24 | 0.25+ |

**Installation:**

```bash
# TI MSP430 GCC
wget https://www.ti.com/tool/download/MSP430-GCC-OPENSOURCE
# Extract and add to PATH

# Or use distribution package
sudo apt install -y gcc-msp430 binutils-msp430 mspdebug
```

### 2.4 Clang/LLVM (Optional, Experimental)

**Targets:** All (AVR support experimental in Clang 17+)

```bash
# Clang 20+ for AVR
sudo apt install -y clang-20 llvm-20

# Verify AVR target
clang-20 --print-targets | grep avr
```

---

## 3. Development Dependencies

### 3.1 Documentation Tools

| Tool | Version | Purpose |
|------|---------|---------|
| **Doxygen** | 1.9.0+ | API documentation |
| **Sphinx** | 4.5.0+ | User documentation |
| **Breathe** | 4.34.0+ | Doxygen-Sphinx bridge |
| **Graphviz** | 2.42+ | Dependency graphs |

**Installation:**

```bash
# System packages
sudo apt install -y doxygen graphviz

# Python packages
pip3 install sphinx breathe sphinx-rtd-theme
```

### 3.2 Testing & Analysis Tools

| Tool | Version | Purpose |
|------|---------|---------|
| **Valgrind** | 3.18+ | Memory leak detection (host tests) |
| **AddressSanitizer** | Built-in | Memory error detection |
| **UBSan** | Built-in | Undefined behavior detection |
| **lcov** | 1.15+ | Code coverage |
| **cppcheck** | 2.10+ | Static analysis |
| **clang-tidy** | 14+ | Linting |

**Installation:**

```bash
sudo apt install -y valgrind lcov cppcheck clang-tidy
```

### 3.3 Simulation & Emulation

| Tool | Version | Purpose |
|------|---------|---------|
| **QEMU** | 8.2+ | AVR emulation (`arduino-uno`) |
| **simavr** | 1.7+ | AVR cycle-accurate simulation |
| **Renode** | 1.13+ | ARM Cortex-M emulation |

**simavr Installation:**

```bash
git clone https://github.com/buserror/simavr.git
cd simavr
make -j$(nproc)
sudo make install
```

---

## 4. Runtime Dependencies (Target-Specific)

### 4.1 AVR Targets

**Flash Programmer:**
- **avrdude** 7.1+ with programmer support:
  - `arduino` (Arduino bootloader via serial)
  - `usbasp` (USBasp programmer)
  - `stk500v2` (AVR ISP mkII)

**Serial Port Access:**
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
# Logout and login for changes to take effect

# Verify port access
ls -l /dev/ttyACM0 /dev/ttyUSB0
```

### 4.2 ARM Targets

**Debug Probe:**
- **ST-Link v2/v3** (STM32)
- **J-Link** (Multi-vendor)
- **CMSIS-DAP** (Generic)

**OpenOCD Configuration:**
```bash
# Test connection
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

---

## 5. Profile-Specific Requirements

### 5.1 Low-End Profile (8-bit minimal)

**Minimum Hardware:**
- 128 bytes RAM
- 4 KB Flash
- 1 UART or GPIO for output

**Software:**
- AVR GCC 7.3+
- Meson 0.56+

**Features Enabled:**
- Scheduler: Simple cooperative
- Max Tasks: 2-4
- No filesystem
- No networking
- No dynamic memory

**Flash Budget:** ≤ 4 KB
**RAM Budget:** ≤ 512 bytes

### 5.2 Mid-Range Profile (enhanced 8/16-bit)

**Minimum Hardware:**
- 4 KB RAM
- 32 KB Flash
- UART, Timer, Optional: SPI/I2C

**Software:**
- AVR GCC 14+ or MSP430 GCC 9+
- Meson 0.56+

**Features Enabled:**
- Scheduler: Preemptive
- Max Tasks: 8-16
- Filesystem: Optional (EEPROM, Flash)
- Networking: Optional (SLIP)
- Dynamic Memory: Limited (256B-1KB)

**Flash Budget:** ≤ 32 KB
**RAM Budget:** ≤ 4 KB

### 5.3 High-End Profile (32-bit)

**Minimum Hardware:**
- 16 KB RAM
- 128 KB Flash
- Rich peripherals (UART, SPI, I2C, Ethernet, USB)

**Software:**
- ARM GCC 10+
- Meson 0.56+
- Optional: RTOS middleware (FreeRTOS, Zephyr compatibility)

**Features Enabled:**
- Scheduler: Full preemptive, multiple policies
- Max Tasks: 32-64
- Filesystem: Full (FAT, LittleFS)
- Networking: Full (lwIP, TCP/UDP)
- Dynamic Memory: Full heap

**Flash Budget:** ≤ 256 KB (recommended)
**RAM Budget:** ≤ 64 KB (recommended)

---

## 6. Build Configuration Options

### 6.1 Meson Options

```bash
meson setup build \
  --cross-file cross/atmega128_gcc14.cross \
  -Dprofile=mid \
  -Dpackages=filesystem,threading \
  -Dflash_limit_bytes=131072 \
  -Ddebug_gdb=true
```

**Available Options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `profile` | choice | `mid` | Target profile: `low`, `mid`, `high` |
| `packages` | array | `[]` | Feature packages to enable |
| `flash_limit_bytes` | int | 30720 | Maximum firmware size (bytes) |
| `flash_limit` | bool | `true` | Enable size gate |
| `debug_gdb` | bool | `false` | Embed on-device GDB stub |
| `san` | bool | `false` | Enable sanitizers (host tests) |
| `cov` | bool | `false` | Enable coverage (host tests) |
| `flash_port` | string | `/dev/ttyACM0` | Serial port for flashing |
| `flash_programmer` | string | `arduino` | Programmer type |

### 6.2 Package Options

**Available Packages:**

| Package | Description | RAM Cost | Flash Cost | Dependencies |
|---------|-------------|----------|------------|--------------|
| `threading` | POSIX threads | 512B | 1KB | `scheduler` |
| `filesystem` | File I/O | 512B-2KB | 2KB-8KB | `block_device` |
| `networking` | Network stack | 4KB-8KB | 8KB-16KB | `timer`, `uart` |
| `usb` | USB device stack | 2KB | 4KB | - |
| `crypto` | Cryptography (AES, SHA) | 256B | 2KB-4KB | - |
| `gui` | Minimal GUI toolkit | 8KB | 16KB | `framebuffer` |

**Example Configuration:**

```bash
# Mid-range with filesystem and threading
meson setup build_mid \
  --cross-file cross/atmega128_gcc14.cross \
  -Dprofile=mid \
  -Dpackages=threading,filesystem

# High-end with full features
meson setup build_high \
  --cross-file cross/stm32f4_gcc12.cross \
  -Dprofile=high \
  -Dpackages=threading,filesystem,networking,usb
```

---

## 7. Environment Variables

```bash
# Toolchain paths (if not in system PATH)
export AVR_GCC_PATH=/opt/avr-gcc-14/bin
export ARM_GCC_PATH=/opt/gcc-arm-none-eabi-12/bin
export PATH="$AVR_GCC_PATH:$ARM_GCC_PATH:$PATH"

# Cross-compilation
export CROSS_COMPILE=avr-
export CC=avr-gcc
export AR=avr-ar
export OBJCOPY=avr-objcopy

# Build flags (optional overrides)
export CFLAGS="-Oz -flto"
export LDFLAGS="-Wl,--gc-sections"

# Flash configuration
export AVRDUDE_PORT=/dev/ttyACM0
export AVRDUDE_PROGRAMMER=arduino
export AVRDUDE_BAUD=115200
```

---

## 8. Verification & Testing

### 8.1 Verify Toolchain Installation

```bash
# Run setup script
sudo ./setup.sh --modern

# Or manually verify
avr-gcc --version          # Should show 14.x or 7.x
arm-none-eabi-gcc --version # Should show 10.x+
meson --version             # Should show 0.56+
qemu-system-avr -version    # Should show 8.2+
```

### 8.2 Build Test

```bash
# Low-end (ATmega328P)
meson setup build_low --cross-file cross/atmega328p_gcc14.cross -Dprofile=low
meson compile -C build_low

# Mid-range (ATmega128)
meson setup build_mid --cross-file cross/atmega128_gcc14.cross -Dprofile=mid
meson compile -C build_mid

# High-end (ARM Cortex-M4)
meson setup build_high --cross-file cross/stm32f4_gcc12.cross -Dprofile=high
meson compile -C build_high
```

### 8.3 Run Tests

```bash
# Host-side unit tests
meson setup build_host
meson test -C build_host -v

# AVR QEMU smoke test
qemu-system-avr -M arduino-uno -bios build_low/unix0.elf -nographic

# Coverage report
meson configure build_host -Dcov=true
meson test -C build_host
lcov --capture --directory build_host --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

---

## 9. Continuous Integration

### 9.1 GitHub Actions Requirements

**.github/workflows/ci.yml:**

```yaml
jobs:
  build-avr:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install AVR toolchain
        run: |
          sudo apt update
          sudo apt install -y gcc-avr avr-libc binutils-avr meson ninja-build
      - name: Build low-end profile
        run: |
          meson setup build --cross-file cross/atmega328p_gcc14.cross -Dprofile=low
          meson compile -C build
      - name: Build mid-range profile
        run: |
          meson setup build_mid --cross-file cross/atmega128_gcc14.cross -Dprofile=mid
          meson compile -C build_mid
      - name: Size check
        run: meson compile -C build size-gate

  build-arm:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install ARM toolchain
        run: |
          sudo apt update
          sudo apt install -y gcc-arm-none-eabi meson ninja-build
      - name: Build high-end profile
        run: |
          meson setup build_arm --cross-file cross/stm32f4_gcc12.cross -Dprofile=high
          meson compile -C build_arm

  test-host:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install test dependencies
        run: |
          sudo apt update
          sudo apt install -y meson ninja-build valgrind lcov
      - name: Run unit tests
        run: |
          meson setup build_test -Dsan=true
          meson test -C build_test -v
      - name: Coverage
        run: |
          meson configure build_test -Dcov=true
          meson test -C build_test
          lcov --capture --directory build_test --output-file coverage.info
      - name: Upload coverage
        uses: codecov/codecov-action@v3
        with:
          files: coverage.info
```

---

## 10. Known Issues & Workarounds

### 10.1 AVR GCC 14 on Ubuntu

**Issue:** Ubuntu does not ship AVR GCC 14 in official repos.

**Workaround:**
1. Use Debian Sid packages (as in `setup.sh`)
2. Use xPack prebuilt binaries
3. Build from source:

```bash
git clone https://github.com/gcc-mirror/gcc.git
cd gcc
git checkout releases/gcc-14.2.0
./contrib/download_prerequisites
mkdir build && cd build
../configure --target=avr --enable-languages=c --disable-nls --disable-libssp
make -j$(nproc)
sudo make install
```

### 10.2 QEMU AVR `arduino-uno` Machine

**Issue:** QEMU < 8.0 lacks `arduino-uno` machine.

**Workaround:**
Build QEMU from source (see Section 2.1) or use `avr6` generic target:

```bash
qemu-system-avr -M avr6 -bios firmware.elf
```

### 10.3 Clang AVR Support

**Issue:** Clang AVR backend is experimental and incomplete.

**Workaround:**
Use GCC for production builds. Clang can be used for static analysis:

```bash
clang-20 --target=avr -mmcu=atmega328p -c src/file.c -o /dev/null \
  -Wall -Wextra -Werror
```

---

## 11. Minimum System Requirements

### 11.1 Development Host

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **OS** | Linux (any distro) | Ubuntu 22.04 LTS |
| **CPU** | 2 cores | 4+ cores |
| **RAM** | 4 GB | 8+ GB |
| **Disk** | 2 GB | 10+ GB (with toolchains) |

### 11.2 Supported Host Platforms

- ✅ **Linux** (x86-64, aarch64)
- ✅ **macOS** (Intel, Apple Silicon)
- ✅ **Windows** (WSL2, Cygwin, MSYS2)
- ⚠️ **BSD** (untested, should work)

---

## 12. License & Compliance

### 12.1 Toolchain Licenses

| Component | License | Notes |
|-----------|---------|-------|
| **GCC** | GPL-3.0 | Runtime exception for compiled code |
| **avr-libc** | BSD | Free for commercial use |
| **newlib** | BSD/GPL mix | Check specific files |
| **Meson** | Apache-2.0 | Build system only |

### 12.2 Avrix License

**MIT License** - Free for commercial and non-commercial use.

---

## 13. Support & Troubleshooting

### 13.1 Common Build Errors

**Error:** `avr-gcc: command not found`
**Fix:** Install AVR toolchain (Section 2.1)

**Error:** `meson: command not found`
**Fix:** `pip3 install meson` or `sudo apt install meson`

**Error:** `Cross compilation target 'atmega328p' not found`
**Fix:** Ensure cross-file exists in `cross/` directory

**Error:** `size-gate failed: firmware exceeds 30720 bytes`
**Fix:** Disable features or increase limit: `-Dflash_limit_bytes=65536`

### 13.2 Getting Help

- **GitHub Issues:** https://github.com/Oichkatzelesfrettschen/Avrix/issues
- **Documentation:** `/docs/`
- **Examples:** `/examples/`

---

## 14. Update History

| Date | Version | Changes |
|------|---------|---------|
| 2025-11-19 | 2.0 | Initial requirements for scalable POSIX system |
| 2025-06-22 | 1.0 | Original µ-UNIX requirements |

---

**Document Maintainer:** Avrix Development Team
**Last Verified:** 2025-11-19
