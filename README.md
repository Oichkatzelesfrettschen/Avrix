````markdown
# Avrix: µ-UNIX for AVR 🍋  
**Project codename:** **Avrix** (styled as **AVR-unIX**).
In the documentation the kernel is also referred to as **µ-UNIX**.
*A ≤ 10 kB C23 nanokernel, wear-levelled log-FS, and lock / RPC suite for the Arduino Uno R3.*


| MCU | Flash | SRAM | EEPROM | Clock |
| --- | ----- | ---- | ------ | ----- |
| **ATmega328P-PU** | 32 KiB | 2 KiB | 1 KiB | 16 MHz |
| **ATmega16U2-MU** | 16 KiB | 512 B | 512 B | 16 MHz → 48 MHz PLL |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions) -- *Snapshot · 20 Jun 2025 – every command below is exercised by CI against this repo and the latest `setup.sh`.*

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern     # GCC-14 + QEMU smoke-boot   (recommended)
# or
sudo ./setup.sh --legacy     # GCC-7.3 only – bare minimum
# add --no-python if you are offline
````

`setup.sh` automatically

* pins **Debian-sid** `gcc-avr-14` (falls back to Ubuntu 7.3 if sid is blocked),
* installs QEMU ≥ 8.2 + Meson + docs & analysis helpers,
* `--no-python` skips the docs helpers for offline installs,
* **builds** the firmware, boots it in QEMU (`arduino-uno` machine),
* prints MCU-specific **CFLAGS / LDFLAGS** you can paste into any Makefile.

---

## 1 · Compiler paths

| Mode       | GCC  | Source                              | ✅ Pros                                             | ⚠️ Cons                         |
| ---------- | ---- | ----------------------------------- | -------------------------------------------------- | ------------------------------- |
| **Modern** | 14.2 | Debian-sid packages *or* xPack 13.x | C23, `-mrelax`, `-mcall-prologues`, smallest flash | Needs apt-pin or `$PATH` tweak  |
| **Legacy** | 7.3  | Ubuntu *universe*                   | Zero extra setup                                   | C11 only, ≈ 8 % larger binaries |

No Launchpad PPA ships AVR GCC ≥ 10 – ignore any guide that mentions `team-gcc-arm-embedded/avr`.

---

### 1A · Debian-sid pin (Modern)

```bash
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
      http://deb.debian.org/debian sid main' \
| sudo tee /etc/apt/sources.list.d/debian-sid-avr.list

sudo tee /etc/apt/preferences.d/90-avr <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF

sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr avrdude gdb-avr qemu-system-misc
```

*Installs `gcc-avr 14.2.0-2` + `avr-libc 2.2`.*

### 1B · xPack tarball (Modern, no root)

```bash
curl -L \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz \
  -o /tmp/avr.tgz
mkdir -p "$HOME/opt/avr" && \
tar -C "$HOME/opt/avr" --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

### 1C · Ubuntu archive (Legacy)

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr avrdude gdb-avr qemu-system-misc
```

### 1D · Optional Clang/LLVM 20

```bash
sudo add-apt-repository -y ppa:llvm-team/llvm-next
sudo apt update
sudo apt install -y clang-20 lld-20 llvm-20
# Meson cross-file: cross/atmega328p_clang20.cross
```

---

## 2 · Developer helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm simavr
pip3 install --user breathe exhale sphinx-rtd-theme
npm  install -g   prettier
```

---

## 3 · Recommended flags (ATmega328P)

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues \
        --icf=safe -fipa-pta"   # last two need GCC ≥ 14
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

Legacy build? Replace `-std=c23` with `-std=c11` and drop the two GCC-14 extras.

---

## 4 · Build & run

```bash
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
meson compile -C build flash           # flash over /dev/ttyACM0
```

For LLVM: use `cross/atmega328p_clang20.cross`.

### 4A · Simavr quick test

```bash
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build nk_elf        # emits build/nk.elf
simavr -m atmega328p build/nk.elf
```

View UART output with verbose mode:

```bash
simavr -m atmega328p -v build/nk.elf
```

Only the AVR core runs – external peripherals are not modelled. See the
[simavr documentation](https://github.com/buserror/simavr/wiki) for details.

### 4A · Custom toolchain prefix

Set `AVR_PREFIX` when the AVR tools live outside `/usr/bin`.  Use the
helper script to regenerate the cross file and point Meson to it:

```bash
AVR_PREFIX=/opt/avr/bin ./cross/gen_avr_cross.sh
meson setup build --wipe --cross-file cross/avr_m328p.txt
```

---

## 5 · Verify install

```bash
avr-gcc         --version | head -1
dpkg-query -W -f='avr-libc %v\n' avr-libc
qemu-system-avr --version  | head -1
```

---

## 6 · Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C            /* GPIOR0 – 1-cycle I/O */
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must live in lower I/O");
```

Override at configure-time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

### 6A · Stack & quantum overrides

```c
#ifndef NK_STACK_SIZE
#define NK_STACK_SIZE 128u           /* bytes per task stack */
#endif
#ifndef NK_QUANTUM_MS
#define NK_QUANTUM_MS 10u            /* round-robin slice */
#endif
```

Override at configure-time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_STACK_SIZE=256 -DNK_QUANTUM_MS=5"
```

---

## 7 · Hardware target

| Chip           | Role            | Clock          | Flash / SRAM | Notes               |
| -------------- | --------------- | -------------- | ------------ | ------------------- |
| **ATmega328P** | Application MCU | 16 MHz crystal | 32 k / 2 k   | classic 8-bit AVRe+ |
| **ATmega16U2** | USB-CDC bridge  | 48 MHz PLL     | 16 k / 512 B | LUFA firmware       |

---

## 8 · What you get

* **Nano-kernel** < 10 kB – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (420 B flash)
* **Door RPC** – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT
* **Unified spinlock** – merges BKL and DAG/Lattice approaches with backward‑compatible aliases
* **Fixed-point Q8.8** helpers
* **Full QEMU board model** (`arduino-uno`) wired into CI

### 8A · Unified spinlock

`nk_spinlock` is a **hybrid‑chimera BKL–DAG matrix** scheme that blends a
global BKL with fine‑grained locks using copy‑on‑write state. It exposes
real‑time lock primitives, and existing code can seamlessly adopt the new
model via the `nk_superlock` compatibility layer.  See
[include/nk_spinlock.h](include/nk_spinlock.h) for API details.

---

## 9 · Contributing

```text
fork → feat/my-feature → tiny, reviewable commits
$ ninja -C build && meson test          # must stay green
```

Document any flash / SRAM delta in **docs/monograph.rst**.

---

## 10 · Filesystem demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates two files in TinyLog-4, reads them back, prints via UART.

---

## 11 · Running the test-suite

```bash
meson test -C build --print-errorlogs
```

Tests using the host CPU run directly. Cross builds leverage **simavr** so the
AVR binaries execute in simulation. Ensure `simavr` is installed and available
in your `$PATH`.

The `spinlock_isr` case stresses the unified spinlock under a 1 kHz timer
interrupt using `simavr`.  It runs automatically when cross compiling.

---

## 12 · Dockerized QEMU test

```bash
docker build -t avrix-qemu docker
docker run --rm -it avrix-qemu
```

The container compiles the firmware, emits `avrix.img`, then boots QEMU.

---

## 13 · Gap & friction backlog

| Gap                                       | Why it matters                                 | Proposed fix                                        |
| ----------------------------------------- | ---------------------------------------------- | --------------------------------------------------- |
| **Real-board flash helper**               | newcomers still need the `avrdude` incantation | `meson compile -C build flash` flashes the Uno |
| **tmux-dev launcher**                     | 4-pane session exists only in docs             | ship `scripts/tmux-dev.sh`                          |
| **On-device GDB stub**                    | “printf + LED” is clumsy                       | gate tiny `avr-gdbstub` behind `-DDEBUG_GDB`        |
| **Static-analysis CI**                    | cppcheck runs locally only                     | ✅ `cppcheck` & `clang-tidy` GitHub job                |
| **Binary-size guardrail**                 | flash creep goes unnoticed                     | Meson `size-gate` custom target (< 30 kB)           |
| *(full table continues in README source)* |                                                |                                                     |

Pull requests welcome – keep each under **1 kB flash**. 🐜
Happy hacking – and keep the footprint smaller than an emoji! 🍋

```

*Everything now renders without conflict markers, the build commands are unified, and the roadmap/​gap table is preserved.*
```
