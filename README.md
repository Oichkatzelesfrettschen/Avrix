
Avrix — µ-UNIX for AVR 🍋
========================

**Codename:** **Avrix** (styled **AVR-unIX**).  
The kernel itself is called **µ-UNIX**.

*A ≤ 10 kB C23 nano-kernel, wear-levelled **TinyLog-4**, and a unified  
spinlock / Door-RPC suite for the Arduino Uno R3.*

| MCU               | Flash | SRAM | EEPROM | Clock            |
| ----------------- | ----- | ---- | ------ | ---------------- |
| **ATmega328P-PU** | 32 kB | 2 kB | 1 kB   | 16 MHz           |
| **ATmega16U2-MU** | 16 kB | 512 B| 512 B  | 16 → 48 MHz PLL  |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)


## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern      # GCC-14 + QEMU smoke-boot (recommended)
sudo ./setup.sh --legacy      # GCC 7.3 – bare minimum
# add --no-python if you are offline
````

`setup.sh` automatically

* pins **Debian-sid** `gcc-avr-14` (falls back to Ubuntu 7.3 if unavailable) \[1];
* installs QEMU ≥ 8.2, Meson, and doc helpers \[2];
* skips Python helpers with `--no-python`;
* **builds** the firmware and boots it in QEMU (`arduino-uno` machine) \[3];
* prints MCU-specific **CFLAGS / LDFLAGS** for easy Makefile drop-in.

---

## 1 · Compiler tracks

| Mode       | GCC  | Source                       | ✅ Advantages                                  | ⚠️ Trade-offs               |
| ---------- | ---- | ---------------------------- | --------------------------------------------- | --------------------------- |
| **Modern** | 14.2 | Debian-sid pkgs / xPack 13.x | C23, `-mrelax`, `-mcall-prologues`, tiny bins | Needs apt-pin or PATH tweak |
| **Legacy** | 7.3  | Ubuntu *universe*            | Zero extra setup                              | C11 only, ≈ 8 % larger bins |

> No Launchpad PPA ships AVR-GCC ≥ 10 — ignore any guide mentioning
> `team-gcc-arm-embedded/avr`. \[4]

---

## 2 · Build & run

```bash
meson setup   build --wipe --cross-file cross/atmega328p_gcc14.cross
# LLVM/Clang users can instead specify
#   cross/atmega328p_clang.cross  (generic) or
#   cross/atmega328p_clang20.cross if Clang 20 is installed
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
meson compile -C build flash          # flashes over /dev/ttyACM0
meson compile -C build size-gate      # fails if firmware exceeds -Dflash_limit
```

Customise the limit with:

```bash
meson configure build -Dflash_limit=32768
```

Logs (`build.log`, `build.log.txt`) are produced by CI (or manually with
`meson compile --log-file build.log | tee build.log.txt`) and ignored by git.

### 2.1 · Developer packages

Install host utilities for debugging and documentation:

```bash
sudo apt install -y valgrind linux-tools-common linux-tools-generic
```

See :ref:`toolchain-setup` for a full list including Meson, Doxygen and
Sphinx helpers.

---

## 3 · What you get

* **Nano-kernel** < 10 kB – 1 kHz pre-emptive round-robin.
* **TinyLog-4** – wear-levelled EEPROM log (≈ 420 B flash).
* **Door RPC** – zero-copy Cap'n-Proto slab, ≈ 1 µs RTT \[5].
* **Unified spinlock** – TAS / quaternion / Beatty-lattice variants with BKL aliases \[6].
* **Fixed-point Q8.8** helpers.
* **Full QEMU board model** (`arduino-uno`) integrated into CI.

### 3.1 · Portable Architecture (Phase 5 Complete)

The codebase has been refactored into a portable embedded POSIX system supporting multiple architectures beyond AVR:

**Hardware Abstraction Layer (HAL):**
- `arch/common/hal.h` - Unified interface for context switching, atomics, memory barriers
- `arch/avr8/` - AVR8 implementation (ATmega128, ATmega328P, etc.)
- `arch/arm/` - ARM Cortex-M support (future)
- `arch/msp430/` - TI MSP430 support (future)
- `arch/x86/` - x86/x64 for host testing (future)

**Portable Kernel Subsystems:**
- `kernel/sched/` - Scheduler (portable, uses HAL for context switching)
- `kernel/sync/` - Spinlocks, mutexes (HAL atomics)
- `kernel/mm/` - Memory allocator (kalloc with tier-based sizing)
- `kernel/ipc/` - Door RPC (portable, HAL memory barriers)

**Driver Layer (3,108 lines, Phase 5):**
- **Filesystems** (1,736 lines):
  - `drivers/fs/romfs.{c,h}` - Read-only memory filesystem (flash/ROM)
  - `drivers/fs/eepfs.{c,h}` - EEPROM filesystem with wear-leveling
  - `drivers/fs/vfs.{c,h}` - Virtual filesystem layer with mount points

- **Networking** (808 lines):
  - `drivers/net/slip.{c,h}` - RFC 1055 SLIP protocol (stateless)
  - `drivers/net/ipv4.{c,h}` - IPv4 stack with RFC 1071 checksum

- **Character Devices** (564 lines):
  - `drivers/tty/tty.{c,h}` - TTY driver with ring buffers

**Novel Optimizations:**
- **VFS Dispatch**: Function pointer table for zero-overhead polymorphism
- **EEPROM Wear-Leveling**: Read-before-write (10-100x life extension)
- **RFC 1071 Checksum**: Fixed carry propagation bug in IPv4
- **Power-of-2 Modulo**: Bitwise AND instead of % (2-10x faster on 8-bit)
- **Header Validation**: IPv4 packet validation before processing

**Memory Footprint (Phase 5 drivers):**
- Flash: ~830 bytes (all drivers combined)
- RAM: ~170 bytes (VFS + descriptors, configurable)
- EEPROM: User files (metadata in flash)

**Status:**
- ✅ Phase 1-4: Foundation (6,236 lines)
- ✅ Phase 5: Driver Migration (3,108 lines)
- ⏭️  Phase 6: Build System Integration (next)

---

## 4 · Repository map generator

`scripts/repo_map.js` scans the code‐base and writes `repo_map.json`.

### 4.1 · Install dependencies

```bash
npm install          # installs fast-glob, p-limit, tree-sitter, tree-sitter-c
```

### 4.2 · Run with custom paths

```bash
node scripts/repo_map.js \
     -s src -s extras -t tests \
     -c cross -o repo_map.json \
     -x 'vendor/**' -j 8
```

* `-s/--src` may be repeated; defaults to `src/`.
* `-t/--tests` chooses the test directory (`tests/` if omitted).
* `-c/--cross` points to the Meson cross-files directory (`cross/` by default).
* `-o/--out` names the output file (`repo_map.json` by default).
* `-x/--exclude` provides fast-glob ignore patterns (repeatable).
* `-j/--jobs` limits parallel parse workers (`0` → all CPUs).

`repo_map.json` captures:

* `generated_at` – ISO time-stamp
* `cross_files` / `toolchains` – names derived from `*.cross` files
* `src_roots`, `tests_dir`, `test_suites`
* per-file function lists for build-bots and static-analysis dashboards

---

## 5 · License

MIT.  See `LICENSE` for details.

---

### References

1. [https://tracker.debian.org/pkg/gcc-avr](https://tracker.debian.org/pkg/gcc-avr)
2. [https://www.qemu.org/2023/12/20/qemu-8-2-0/](https://www.qemu.org/2023/12/20/qemu-8-2-0/)
3. [https://arduino.stackexchange.com/q/95932](https://arduino.stackexchange.com/q/95932)
4. [https://apt.llvm.org/](https://apt.llvm.org/)
5. [https://capnproto.org/rpc.html](https://capnproto.org/rpc.html)
6. [https://en.wikipedia.org/wiki/Beatty\_sequence](https://en.wikipedia.org/wiki/Beatty_sequence)

```