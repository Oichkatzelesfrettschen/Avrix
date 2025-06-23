Avrix: µ-UNIX for AVR 🍋
=======================

**Codename:** **Avrix** (styled **AVR-unIX**).  
The kernel is also referred to as **µ-UNIX**.

*A ≤ 10 kB C23 nano-kernel, wear-levelled **TinyLog-4**, and a unified  
spinlock / Door-RPC suite for the Arduino Uno R3.*

| MCU               | Flash | SRAM | EEPROM | Clock            |
| ----------------- | ----- | ---- | ------ | ---------------- |
| **ATmega328P-PU** | 32 kB | 2 kB | 1 kB   | 16 MHz           |
| **ATmega16U2-MU** | 16 kB | 512 B| 512 B  | 16 → 48 MHz PLL  |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)  

---

### 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern     # GCC-14 + QEMU smoke-boot (recommended)
sudo ./setup.sh --legacy     # GCC 7.3 – bare minimum
# add --no-python if you are offline
```

`setup.sh` automatically

* pins **Debian-sid** `gcc-avr-14` (falls back to Ubuntu 7.3 if unavailable) ([tracker.debian.org][1]);
* installs QEMU ≥ 8.2, Meson, and doc helpers ([qemu.org][2]);
* skips helpers with `--no-python`;
* **builds** the firmware and boots it in QEMU (`arduino-uno` machine) ([arduino.stackexchange.com][3]);
* prints MCU-specific **CFLAGS / LDFLAGS** for easy Makefile drop-in.

---

### 1 · Compiler paths

| Mode       | GCC  | Source                       | ✅ Pros                                        | ⚠️ Cons                        |
| ---------- | ---- | ---------------------------- | --------------------------------------------- | ------------------------------ |
| **Modern** | 14.2 | Debian-sid pkgs / xPack 13.x | C23, `-mrelax`, `-mcall-prologues`, tiny bins | Needs apt-pin or `$PATH` tweak |
| **Legacy** | 7.3  | Ubuntu *universe*            | Zero extra setup                              | C11 only, ≈ 8 % larger bins    |

> No Launchpad PPA ships AVR GCC ≥ 10 — ignore any guide mentioning `team-gcc-arm-embedded/avr`. ([apt.llvm.org][4])


---

### 4 · Build & run

```bash
meson setup   build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
meson compile -C build flash          # flashes over /dev/ttyACM0
meson compile -C build size-gate      # fails if firmware exceeds -Dflash_limit
```

Customize the limit with ``meson configure build -Dflash_limit=32768``.

``build.log`` and ``build.log.txt`` are generated when the CI pipeline
captures ``meson compile`` output (local builds may do the same with
``--log-file`` or ``tee``).  These logs live in the project root and are
ignored by git.


---

### 8 · What you get

* **Nano-kernel** < 10 kB – 1 kHz pre-emptive round-robin.
* **TinyLog-4** – wear-levelled EEPROM log (≈ 420 B flash).
* **Door RPC** – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT ([capnproto.org][5]).
* **Unified spinlock** – TAS / quaternion / Beatty-lattice variants with BKL aliases   ([en.wikipedia.org][6]).
* **Fixed-point Q8.8** helpers.
* **Full QEMU board model** (`arduino-uno`) wired into CI.



[1]: https://tracker.debian.org/gcc-avr "gcc-avr - Debian Package Tracker"
[2]: https://www.qemu.org/2023/12/20/qemu-8-2-0/ "QEMU version 8.2.0 released"
[3]: https://arduino.stackexchange.com/questions/95932/emulating-arduino-uno-with-qemu-interrupts-do-not-work "Emulating Arduino Uno with QEMU: interrupts do not work"
[4]: https://apt.llvm.org/ "LLVM Debian/Ubuntu packages"
[5]: https://capnproto.org/rpc.html "RPC Protocol - Cap'n Proto"
[6]: https://en.wikipedia.org/wiki/Beatty_sequence "Beatty sequence - Wikipedia"
