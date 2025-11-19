# Arduino and Arduino-Compatible Chipset Specifications

**Purpose:** Reference guide for selecting target microcontrollers for Avrix POSIX profiles

**Last Updated:** 2025-01-19

**See Also:**
- [docs/technical/ATMEGA328P_REFERENCE.md](docs/technical/ATMEGA328P_REFERENCE.md) - Deep-dive ATmega328P guide
- [docs/technical/ATMEGA32U4_REFERENCE.md](docs/technical/ATMEGA32U4_REFERENCE.md) - Deep-dive ATmega32U4 guide
- [docs/technical/CHIPSET_COMPARISON.md](docs/technical/CHIPSET_COMPARISON.md) - Performance comparison & decision trees

---

## Table of Contents

1. [Overview](#overview)
2. [Chipsets by RAM Size](#chipsets-by-ram-size)
3. [Detailed Specifications](#detailed-specifications)
4. [PSE Profile Recommendations](#pse-profile-recommendations)
5. [Arduino Board Examples](#arduino-board-examples)

---

## Overview

This document catalogs Arduino and Arduino-compatible microcontrollers organized by RAM capacity, which is the primary constraint for embedded POSIX implementations. Flash and EEPROM sizes are also documented for completeness.

**Key Memory Types:**
- **SRAM**: Volatile memory for runtime data (stack, heap, global variables)
- **Flash**: Non-volatile program memory
- **EEPROM**: Non-volatile data storage (config, persistent state)

---

## Chipsets by RAM Size

### 1KB SRAM - Ultra-Low Resource

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| ATmega168     | 16KB   | 512B   | 1KB   | 16 MHz | AVR 8-bit    | Arduino Nano (old)   |
| ATtiny85      | 8KB    | 512B   | 512B  | 8 MHz  | AVR 8-bit    | Digispark            |
| ATtiny84      | 8KB    | 512B   | 512B  | 8 MHz  | AVR 8-bit    | Various ATtiny boards|

**Avrix Compatibility:** Too small for PSE51 (requires 1-2KB RAM minimum)

---

### 2KB SRAM - PSE51 Target (Minimal Profile)

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **ATmega328** | 32KB   | 1KB    | 2KB   | 16 MHz | AVR 8-bit    | Arduino Uno          |
| **ATmega328P**| 32KB   | 1KB    | 2KB   | 16 MHz | AVR 8-bit    | Arduino Uno R3, Nano |
| **ATmega328PB**| 32KB  | 1KB    | 2KB   | 16 MHz | AVR 8-bit    | Arduino Nano Every   |

**Avrix Compatibility:** ✅ **PSE51 (Minimal Profile)**
- Single-threaded cooperative scheduling
- Basic I/O (ROMFS, EEPFS)
- No networking (optional with tight constraints)
- Target applications: Data loggers, simple sensors, LED controllers

**Real-World Example:**
- Arduino Uno R3 (ATmega328P)
- Flash: 32KB (firmware ~8-12KB, leaving 20KB for applications)
- RAM: 2KB (kernel ~800B, leaving ~1.2KB for application)

---

### 2.5KB SRAM - PSE51 Extended

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **ATmega32U4**| 32KB   | 1KB    | 2.5KB | 16 MHz | AVR 8-bit    | Arduino Leonardo     |
|               |        |        |       |        |              | Arduino Micro        |
|               |        |        |       |        |              | SparkFun Pro Micro   |

**Avrix Compatibility:** ✅ **PSE51 Extended**
- Single-threaded with USB support
- Built-in USB communication (HID, CDC)
- Can emulate mouse/keyboard
- Slightly more headroom than ATmega328

**Unique Feature:** Native USB support (no separate USB-to-serial chip required)

---

### 4KB SRAM - PSE52 Candidate (Multi-Threaded)

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **ATmega644** | 64KB   | 2KB    | 4KB   | 16 MHz | AVR 8-bit    | Pandauino 644        |
| **ATmega644P**| 64KB   | 2KB    | 4KB   | 16 MHz | AVR 8-bit    | Sanguino             |
|               |        |        |       |        |              | Mighty 644           |

**Avrix Compatibility:** ✅ **PSE52 (Limited Multi-Threading)**
- 2-3 concurrent threads feasible
- Basic networking (SLIP + IPv4)
- Limited IPC (Door RPC)
- Requires careful memory management

**Arduino Compatibility:** Requires third-party cores (e.g., MightyCore)

**Applications:**
- 3D printer controllers (Marlin firmware)
- Multi-sensor data acquisition
- Simple networked devices

---

### 8KB SRAM - PSE52 Target (Multi-Threaded Profile)

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **ATmega2560**| 256KB  | 4KB    | 8KB   | 16 MHz | AVR 8-bit    | Arduino Mega 2560    |
| **ATmega2561**| 256KB  | 4KB    | 8KB   | 16 MHz | AVR 8-bit    | -                    |

**Avrix Compatibility:** ✅ **PSE52 (Multi-Threaded Profile)**
- 4-8 concurrent threads
- Full networking stack (SLIP, IPv4, UDP)
- IPC (Door RPC, spinlocks, mutexes)
- VFS with multiple filesystems
- Real-time task scheduling

**Real-World Example:**
- Arduino Mega 2560
- Flash: 256KB (plenty for complex applications)
- RAM: 8KB (kernel ~2KB, leaving ~6KB for application)

**Applications:**
- CNC controllers
- Robotics platforms
- Multi-protocol gateways
- Complex automation systems

---

### 16KB SRAM - PSE52 Full (Multi-Threaded + Advanced IPC)

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **ATmega1284**| 128KB  | 4KB    | 16KB  | 16 MHz | AVR 8-bit    | Pandauino 1284       |
| **ATmega1284P**| 128KB | 4KB    | 16KB  | 16 MHz | AVR 8-bit    | Mighty 1284P         |
|               |        |        |       |        |              | Bobuino              |

**Avrix Compatibility:** ✅ **PSE52 Full (Most Capable AVR8)**
- 8-16 concurrent threads
- Full networking with protocol stacks
- Advanced IPC mechanisms
- Large application space
- Best AVR8 choice for embedded POSIX

**Arduino Compatibility:** Requires third-party cores (MightyCore)

**Note:** ATmega1284P has twice the RAM of ATmega2560 despite less flash

**Applications:**
- Embedded web servers
- Multi-threaded data processing
- Complex control systems
- Gateway devices

---

### 32KB SRAM - PSE54 Candidate (32-bit ARM)

| Chipset       | Flash  | EEPROM | SRAM  | Clock  | Architecture | Arduino Board        |
|---------------|--------|--------|-------|--------|--------------|----------------------|
| **SAMD21G18** | 256KB  | -      | 32KB  | 48 MHz | ARM Cortex-M0+| Arduino Zero        |
|               |        |        |       |        |              | Arduino MKR WiFi 1010|
|               |        |        |       |        |              | Arduino MKR series   |
|               |        |        |       |        |              | Adafruit Feather M0  |

**Avrix Compatibility:** ✅ **PSE54 Candidate (Limited MMU)**
- 32-bit architecture with better performance
- No MMU (process isolation via MPU only)
- 48 MHz clock (3x faster than AVR8)
- Many concurrent threads feasible
- Modern peripherals (USB, I2C, SPI, CAN)

**Limitations:** No full MMU, so fork/exec limited

**Applications:**
- IoT devices
- Wearables
- Audio processing
- Motor control

---

### 80KB+ SRAM - PSE52/PSE54 Target (ESP Series)

| Chipset       | Flash  | EEPROM | SRAM  | Clock    | Architecture | Arduino Board        |
|---------------|--------|--------|-------|----------|--------------|----------------------|
| **ESP8266**   | 4MB    | -      | 80KB* | 80 MHz   | Xtensa L106  | NodeMCU ESP8266      |
|               |(ext)   |        |       | (160 MHz)|              | Wemos D1 Mini        |
|               |        |        |       |          |              | ESP-01, ESP-12       |

**SRAM Breakdown (ESP8266):**
- 32KB instruction RAM
- 80KB user data RAM (heap + stack)
- 16KB IRAM for cache

**Avrix Compatibility:** ✅ **PSE52 Full + WiFi**
- Built-in WiFi (802.11 b/g/n)
- TCP/IP stack
- Large RAM for many threads
- Arduino IDE support via ESP8266 core

**Applications:**
- WiFi sensors
- Home automation
- IoT devices
- Web servers

---

### 520KB SRAM - PSE54 Full (32-bit Dual-Core)

| Chipset       | Flash  | EEPROM | SRAM  | Clock    | Architecture | Arduino Board        |
|---------------|--------|--------|-------|----------|--------------|----------------------|
| **ESP32**     | 4MB-16MB| -     | 520KB | 160-240 MHz| Xtensa LX6  | ESP32 DevKit         |
|               |(ext)   |        |       |(dual-core)|              | ESP32-WROOM-32       |
|               |        |        |       |          |              | M5Stack              |

**SRAM Breakdown (ESP32):**
- 520KB internal SRAM (DRAM + IRAM)
- Up to 8MB external PSRAM (optional)
- Dual-core Xtensa LX6 @ 240 MHz

**Avrix Compatibility:** ✅ **PSE54 Full (No MMU, but ample resources)**
- Dual-core processing (SMP potential)
- WiFi + Bluetooth
- Massive RAM for complex applications
- Modern peripherals (CAN, Ethernet, SD)
- FreeRTOS already integrated

**Applications:**
- Edge computing
- Video processing
- Multi-protocol gateways
- Embedded Linux alternatives
- Industrial automation

---

### 256KB+ SRAM - PSE54 Full (ARM Cortex-A)

| Chipset       | Flash  | DDR RAM| SRAM  | Clock    | Architecture | Arduino Board        |
|---------------|--------|--------|-------|----------|--------------|----------------------|
| **RP2040**    | 2MB    | -      | 264KB | 133 MHz  | ARM Cortex-M0+| Raspberry Pi Pico   |
|               |(ext)   |        |       |(dual-core)|              | Arduino Nano RP2040  |

**Avrix Compatibility:** ✅ **PSE52/PSE54 Candidate**
- Dual-core ARM Cortex-M0+ @ 133 MHz
- 264KB SRAM (on-chip)
- No MMU, but dual-core capable
- Programmable I/O (PIO) for custom protocols

**Note:** Raspberry Pi (BCM2835/2836) with 512MB-4GB RAM would be full PSE54, but not Arduino-compatible

---

## Detailed Specifications

### AVR 8-bit Architecture (Atmel/Microchip)

**Common Features:**
- Harvard architecture (separate program/data memory)
- RISC instruction set (120-130 instructions)
- Single-cycle execution for most instructions
- Hardware multiply (2 cycles)
- 32 general-purpose registers

**Memory Access:**
- Flash: Program memory (PROGMEM)
- SRAM: Data memory (stack, heap, globals)
- EEPROM: Non-volatile data (100k write cycles)

**Avrix HAL Requirements:**
- Context switching: 32 registers + SREG + PC (35-37 bytes/task)
- Atomics: Interrupt disable (cli/sei)
- PROGMEM: `pgm_read_byte()` for flash data
- EEPROM: `eeprom_read_byte()`, `eeprom_update_byte()`

---

### ARM Cortex-M0/M0+ (32-bit)

**Common Features:**
- Von Neumann architecture (unified memory)
- Thumb instruction set (16/32-bit mixed)
- Hardware divide (2-12 cycles)
- Optional MPU (Memory Protection Unit)
- Nested Vectored Interrupt Controller (NVIC)

**Memory Access:**
- Flash: Memory-mapped (no special PROGMEM needed)
- SRAM: Unified data/stack memory
- No EEPROM (emulated via flash sectors)

**Avrix HAL Requirements:**
- Context switching: 16 registers (64 bytes/task)
- Atomics: LDREX/STREX (M3+) or interrupt disable (M0)
- PROGMEM: Identity functions (no-op)
- EEPROM: Flash emulation with wear-leveling

---

### Xtensa LX6/LX7 (ESP Series)

**Common Features:**
- 32-bit RISC architecture
- Configurable instruction set
- Hardware multiply/divide
- Dual-core (ESP32) or single-core (ESP8266)
- Integrated WiFi/Bluetooth

**Memory Access:**
- Flash: Memory-mapped (cached)
- SRAM: DRAM (data) + IRAM (instructions)
- External PSRAM: Optional (ESP32)

**Avrix HAL Requirements:**
- Context switching: Xtensa-specific (window registers)
- Atomics: Hardware CAS instructions
- PROGMEM: Memory-mapped flash
- EEPROM: NVS (Non-Volatile Storage) library

---

## PSE Profile Recommendations

### PSE51 - Minimal Profile

**Target Hardware:**
- **Minimum**: 2KB RAM, 16KB flash
- **Recommended**: ATmega328P (2KB RAM, 32KB flash)

**Supported Chipsets:**
- ✅ ATmega328/328P/328PB (2KB SRAM) - **Primary target**
- ✅ ATmega32U4 (2.5KB SRAM) - **With USB support**
- ⚠️ ATmega168 (1KB SRAM) - **Marginal, very tight**

**Features:**
- Single-threaded cooperative scheduling
- Basic I/O (ROMFS, EEPFS)
- Optional SLIP networking (tight constraints)
- No IPC, no threading

**Memory Budget:**
```
Flash: 32KB
  - Bootloader: 0.5KB
  - Avrix kernel: 8-12KB
  - Application: 19-23KB

SRAM: 2KB
  - Kernel data: 0.4KB
  - Main stack: 0.4KB
  - Heap: 1.2KB
```

---

### PSE52 - Multi-Threaded Profile

**Target Hardware:**
- **Minimum**: 4KB RAM, 64KB flash
- **Recommended**: ATmega1284P (16KB RAM, 128KB flash)

**Supported Chipsets:**
- ⚠️ ATmega644P (4KB SRAM) - **Limited (2-3 threads)**
- ✅ ATmega2560 (8KB SRAM) - **Good (4-8 threads)**
- ✅ ATmega1284P (16KB SRAM) - **Best AVR8 (8-16 threads)**
- ✅ ESP8266 (80KB SRAM) - **Excellent (WiFi included)**
- ✅ ESP32 (520KB SRAM) - **Overkill but compatible**

**Features:**
- Preemptive multi-threading (pthread)
- Networking (SLIP, IPv4, UDP)
- IPC (Door RPC, mutexes, spinlocks)
- VFS with multiple filesystems
- Real-time scheduling

**Memory Budget (ATmega1284P):**
```
Flash: 128KB
  - Bootloader: 0.5KB
  - Avrix kernel: 20-30KB
  - Application: 97-107KB

SRAM: 16KB
  - Kernel data: 2KB
  - TCB (8 tasks @ 128B): 1KB
  - Stacks (8 tasks @ 512B): 4KB
  - Heap: 9KB
```

---

### PSE54 - Full POSIX Profile

**Target Hardware:**
- **Minimum**: 16MB RAM, 128KB flash, MMU
- **Recommended**: ARM Cortex-A (512MB+ RAM)

**Supported Chipsets:**
- ⚠️ SAMD21 (32KB SRAM) - **No MMU, limited processes**
- ⚠️ RP2040 (264KB SRAM) - **No MMU, dual-core only**
- ⚠️ ESP32 (520KB SRAM) - **No MMU, but massive resources**
- ✅ ARM Cortex-A7/A53 (512MB-4GB) - **Full PSE54 with MMU**

**Features:**
- Process management (fork, exec, wait)
- Virtual memory (MMU-based)
- Full signal support
- Shared memory (mmap)
- Complete IPC suite (pipes, sockets, signals)

**Note:** Most Arduino boards lack MMU, so full PSE54 requires stepping up to embedded Linux targets (Raspberry Pi, BeagleBone, etc.)

**Memory Budget (Cortex-A7 @ 512MB):**
```
Flash: 128KB-4MB
  - Bootloader: 64KB
  - Avrix kernel: 128KB
  - Application: Variable

RAM: 512MB
  - Kernel: 16MB
  - Process space: 496MB
  - Per-process overhead: ~1MB
```

---

## Arduino Board Examples

### Official Arduino Boards

| Board              | Chipset        | SRAM  | Flash  | PSE Profile | Notes                    |
|--------------------|----------------|-------|--------|-------------|--------------------------|
| Arduino Uno R3     | ATmega328P     | 2KB   | 32KB   | PSE51       | Most popular, ubiquitous |
| Arduino Nano       | ATmega328P     | 2KB   | 32KB   | PSE51       | Breadboard-friendly      |
| Arduino Leonardo   | ATmega32U4     | 2.5KB | 32KB   | PSE51+      | Native USB HID           |
| Arduino Micro      | ATmega32U4     | 2.5KB | 32KB   | PSE51+      | Smaller Leonardo         |
| Arduino Mega 2560  | ATmega2560     | 8KB   | 256KB  | PSE52       | Many I/O pins            |
| Arduino Zero       | SAMD21G18      | 32KB  | 256KB  | PSE52/54*   | 32-bit ARM, no MMU       |
| Arduino MKR WiFi 1010| SAMD21 + ESP32| 32KB  | 256KB  | PSE52       | WiFi via ESP32 module    |
| Arduino Nano RP2040| RP2040         | 264KB | 2MB    | PSE52/54*   | Dual-core, no MMU        |
| Arduino Uno R4 Minima| RA4M1 (ARM)  | 32KB  | 256KB  | PSE52       | New 2024 model           |

*Limited PSE54 (no MMU for full process isolation)

---

### Third-Party Compatible Boards

| Board              | Chipset        | SRAM  | Flash  | PSE Profile | Notes                    |
|--------------------|----------------|-------|--------|-------------|--------------------------|
| Pandauino 644      | ATmega644P     | 4KB   | 64KB   | PSE52*      | Requires MightyCore      |
| Pandauino 1284     | ATmega1284P    | 16KB  | 128KB  | PSE52       | Best AVR8 for Avrix      |
| Sanguino           | ATmega644P     | 4KB   | 64KB   | PSE52*      | 3D printer boards        |
| NodeMCU ESP8266    | ESP8266        | 80KB  | 4MB    | PSE52       | WiFi built-in            |
| ESP32 DevKit       | ESP32          | 520KB | 4MB    | PSE52/54*   | Dual-core, WiFi+BT       |
| SparkFun Pro Micro | ATmega32U4     | 2.5KB | 32KB   | PSE51+      | Leonardo clone           |
| Raspberry Pi Pico  | RP2040         | 264KB | 2MB    | PSE52/54*   | Dual-core Cortex-M0+     |
| Adafruit Feather M0| SAMD21G18      | 32KB  | 256KB  | PSE52       | Compact form factor      |

*Limited due to memory constraints or lack of MMU

---

## Cross-Compilation Setup

### ATmega328P (Arduino Uno)

```bash
# Install toolchain
sudo apt install gcc-avr avr-libc avrdude

# Build for ATmega328P
meson setup build_uno --cross-file cross/atmega328p_gcc14.cross
meson compile -C build_uno

# Flash via Arduino bootloader
avrdude -c arduino -p m328p -P /dev/ttyACM0 -b 115200 \
        -U flash:w:build_uno/unix0.hex:i
```

### ATmega1284P (Pandauino)

```bash
# Build for ATmega1284P
meson setup build_1284 --cross-file cross/atmega1284_gcc14.cross
meson compile -C build_1284

# Flash via ISP programmer
avrdude -c usbasp -p m1284p -U flash:w:build_1284/unix0.hex:i
```

### SAMD21 (Arduino Zero)

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi

# Build for SAMD21
meson setup build_zero --cross-file cross/samd21_gcc.cross
meson compile -C build_zero

# Flash via BOSSA
bossac -p /dev/ttyACM0 -e -w -v -b build_zero/unix0.bin
```

### ESP32 (Arduino IDE)

```bash
# Install ESP-IDF or Arduino-ESP32
# ESP32 requires special build system (not Meson)

# Via PlatformIO
pio run -e esp32dev -t upload
```

---

## Performance Comparison

### Context Switch Latency (Estimated)

| Chipset        | Clock    | Cycles | Time (µs) | Notes                    |
|----------------|----------|--------|-----------|--------------------------|
| ATmega328P     | 16 MHz   | 35-40  | 2.2-2.5   | 32 registers + SREG + PC |
| ATmega1284P    | 16 MHz   | 35-40  | 2.2-2.5   | Same AVR8 architecture   |
| ATmega32U4     | 16 MHz   | 35-40  | 2.2-2.5   | Same AVR8 architecture   |
| SAMD21         | 48 MHz   | 20-30  | 0.4-0.6   | 16 ARM registers         |
| ESP8266        | 80 MHz   | 50-100 | 0.6-1.2   | Xtensa window registers  |
| ESP32          | 240 MHz  | 50-100 | 0.2-0.4   | Dual-core Xtensa         |
| RP2040         | 133 MHz  | 20-30  | 0.15-0.23 | Dual-core Cortex-M0+     |

**Interpretation:** Faster context switching allows more responsive multi-threading

---

### Throughput (MIPS - Million Instructions Per Second)

| Chipset        | Clock    | MIPS   | Notes                    |
|----------------|----------|--------|--------------------------|
| ATmega328P     | 16 MHz   | 16     | 1 cycle/instruction avg  |
| ATmega1284P    | 16 MHz   | 16     | Same AVR8 architecture   |
| SAMD21         | 48 MHz   | 36-48  | ARM Thumb instructions   |
| ESP8266        | 80-160MHz| 80-160 | Xtensa configurable      |
| ESP32          | 240 MHz  | 400-600| Dual-core + cache        |
| RP2040         | 133 MHz  | 200-266| Dual-core Cortex-M0+     |

**Interpretation:** Higher MIPS = more compute capacity for algorithms

---

## Conclusion

**Best Chipset Recommendations by Profile:**

- **PSE51 (Minimal)**: ATmega328P (2KB RAM)
  - Ubiquitous, well-supported, perfect for learning
  - Arduino Uno R3 is the gold standard

- **PSE52 (Multi-Threaded)**: ATmega1284P (16KB RAM)
  - Best AVR8 for multi-threading
  - 2x RAM of ATmega2560 with less flash
  - Requires third-party cores (MightyCore)

- **PSE52 (With WiFi)**: ESP32 (520KB RAM)
  - Massive resources, WiFi + Bluetooth
  - Dual-core 240 MHz
  - Arduino IDE support via ESP32 core

- **PSE54 (Full POSIX)**: Embedded Linux (BCM2835+, 512MB+ RAM)
  - Raspberry Pi, BeagleBone
  - Full MMU support for process isolation
  - Not Arduino, but full POSIX compatible

**Migration Path:**
1. Start with Arduino Uno (PSE51) for prototyping
2. Scale to ATmega1284P (PSE52) for multi-threading
3. Add WiFi with ESP32 (PSE52+) for IoT
4. Move to embedded Linux (PSE54) for complex systems

---

*Last Updated: 2025-01-19 (Post-Phase 8)*
*References: Microchip datasheets, Arduino.cc, Espressif documentation, Raspberry Pi Foundation*
