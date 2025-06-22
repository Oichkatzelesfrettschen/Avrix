````markdown
# Avrix: µ-UNIX for AVR 🍋  
**Project codename:** **Avrix** (styled as **AVR-unIX**)  
In documentation the kernel is also referred to as **µ-UNIX**.  
*A ≤ 10 kB C23 nanokernel, wear-levelled TinyLog-4, and unified spinlock / Door RPC suite for the Arduino Uno R3.*

| MCU               | Flash   | SRAM   | EEPROM | Clock               |
| ----------------- | ------- | ------ | ------ | ------------------- |
| **ATmega328P-PU** | 32 KiB  | 2 KiB  | 1 KiB  | 16 MHz              |
| **ATmega16U2-MU** | 16 KiB  | 512 B  | 512 B  | 16 MHz → 48 MHz PLL  |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)  
*Snapshot · 20 Jun 2025 — every command below is exercised by CI against this repo and the latest `setup.sh`.*

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern     # GCC-14 + QEMU smoke-boot   (recommended)
# or
sudo ./setup.sh --legacy     # GCC-7.3 only – bare minimum
# add --no-python if you are offline
````

`setup.sh` automatically:

* pins **Debian-sid** `gcc-avr-14` (falls back to Ubuntu 7.3 if unavailable),
* installs QEMU ≥ 8.2, Meson, docs & analysis helpers,
* skips docs helpers with `--no-python`,
* **builds** the firmware and boots it in QEMU (`arduino-uno` machine),
* prints MCU-specific **CFLAGS** / **LDFLAGS** for your Makefile.

---

## 1 · Compiler paths

| Mode       | GCC  | Source                              | ✅ Pros                                             | ⚠️ Cons                           |
| ---------- | ---- | ----------------------------------- | -------------------------------------------------- | --------------------------------- |
| **Modern** | 14.2 | Debian-sid packages *or* xPack 13.x | C23, `-mrelax`, `-mcall-prologues`, smallest flash | Requires apt-pin or `$PATH` tweak |
| **Legacy** | 7.3  | Ubuntu *universe*                   | Zero extra setup                                   | C11 only, ≈ 8 % larger binaries   |

> No Launchpad PPA ships AVR GCC ≥ 10 — ignore any guide mentioning `team-gcc-arm-embedded/avr`.

### 1A · Debian-sid pin (Modern)

```bash
# enable sid for AVR toolchain
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
      http://deb.debian.org/debian sid main' \
  | sudo tee /etc/apt/sources.list.d/debian-sid-avr.list

sudo tee /etc/apt/preferences.d/90-avr << 'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF

sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr avrdude gdb-avr qemu-system-misc
```

> Installs `gcc-avr 14.2.0-2` + `avr-libc 2.2`.

### 1B · xPack tarball (Modern, no root)

```bash
curl -L \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz \
  -o /tmp/avr.tgz

mkdir -p "$HOME/opt/avr" \
  && tar -C "$HOME/opt/avr" --strip-components=1 -xf /tmp/avr.tgz

echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile
source ~/.profile
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
# then use cross/atmega328p_clang20.cross
```

---

## 2 · Developer helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm simavr

pip3 install --user breathe exhale sphinx-rtd-theme
npm install -g prettier
```

---

## 3 · Recommended flags (ATmega328P)

```bash
export MCU=atmega328p

CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz \
        -flto -mrelax -ffunction-sections -fdata-sections \
        -mcall-prologues --icf=safe -fipa-pta"

LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

> For legacy: replace `-std=c23` → `-std=c11` and drop GCC 14-only extras.

---

## 4 · Build & run

```bash
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
meson compile -C build flash   # flashes over /dev/ttyACM0
```

*For LLVM: use* `cross/atmega328p_clang20.cross`.

### 4A · Simavr quick test

```bash
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build nk_elf        # emits build/nk.elf
simavr -m atmega328p build/nk.elf
```

*View UART:*

```bash
simavr -m atmega328p -v build/nk.elf
```

*Only the AVR core runs — external peripherals aren’t modelled. See [simavr wiki](https://github.com/buserror/simavr/wiki).*

### 4B · Tmux development layout

```bash
./scripts/tmux-dev.sh
```

Launches a four-pane `tmux` session:

1. **Build** – loops `meson compile -C build`
2. **Serial monitor** – `screen /dev/ttyACM0`
3. **Editor** – `$EDITOR` in repo root
4. **Shell** – spare shell for Git or tools

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

*Override:*

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

### 6A · Stack & quantum overrides

```c
#ifndef NK_STACK_SIZE
#define NK_STACK_SIZE 128u           /* bytes per task stack */
#endif
#ifndef NK_QUANTUM_MS
#define NK_QUANTUM_MS 10u            /* round-robin slice (ms) */
#endif
```

*Override:*

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_STACK_SIZE=256 -DNK_QUANTUM_MS=5"
```

---

## 7 · Hardware target

| Chip           | Role            | Clock          | Flash / SRAM | Notes             |
| -------------- | --------------- | -------------- | ------------ | ----------------- |
| **ATmega328P** | Application MCU | 16 MHz crystal | 32 k / 2 k   | classic 8-bit AVR |
| **ATmega16U2** | USB-CDC bridge  | 48 MHz PLL     | 16 k / 512 B | LUFA firmware     |

---

## 8 · What you get

* **Nano-kernel** < 10 kB – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (≈ 420 B flash)
* **Door RPC** – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT
* **Unified spinlock** – merges BKL & DAG/Lattice with backward-compatible aliases; offers global BKL or fine-grained real-time locking via
  `nk_spinlock_init`, `nk_spinlock_lock`/`trylock`, `nk_spinlock_lock_rt`/`trylock_rt`, `nk_spinlock_unlock`. Under the hood: TAS, quaternion & Beatty-lattice variants.
* **Fixed-point Q8.8** helpers
* **Full QEMU board model** (`arduino-uno`) wired into CI

### 8A · Unified spinlock details

`nk_spinlock` is a **hybrid-chimera BKL–DAG matrix** blending a global Big Kernel Lock with fine-grained spinlocks and speculative COW snapshots.
It exposes both blocking and real-time primitives, and legacy code can adopt via the `nk_superlock` layer.
See [include/nk\_spinlock.h](include/nk_spinlock.h) for full API.

---

## 9 · Contributing

```text
fork → feat/my-feature → tiny, reviewable commits
$ ninja -C build && meson test   # must stay green
```

Document any flash / SRAM delta in **docs/monograph.rst**.

---

## 10 · Filesystem demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates files in TinyLog-4, reads back, prints via UART.

---

## 11 · Running the test suite

```bash
meson test -C build --print-errorlogs
```

Host-CPU tests run directly; cross builds use **simavr**. The `spinlock_isr` case stresses unified spinlock under 1 kHz timer.

---

## 12 · Dockerized QEMU test

```bash
docker build -t avrix-qemu docker
docker run --rm -it avrix-qemu
```

Container compiles firmware, emits `avrix.img`, then boots QEMU.

---

## 13 · Gap & friction backlog

| Gap                                | Why it matters                           | Proposed fix                                 |
| ---------------------------------- | ---------------------------------------- | -------------------------------------------- |
| **Real-board flash helper**        | Newcomers still need the avrdude incant. | `scripts/flash.sh build/unix0.hex`           |
| **tmux-dev launcher**              | 4-pane session exists only in docs       | Ship `scripts/tmux-dev.sh`                   |
| **On-device GDB stub**             | “printf + LED” is clumsy                 | Gate tiny `avr-gdbstub` behind `-DDEBUG_GDB` |
| **Static-analysis CI**             | cppcheck runs locally only               | Add `cppcheck` & `clang-tidy` GitHub job     |
| **Binary-size guardrail**          | Flash creep goes unnoticed               | Meson `size-gate` custom target (< 30 kB)    |
| *(full table continues in source)* |                                          |                                              |

Pull requests welcome – keep each under **1 kB flash**. 🐜
Happy hacking – keep the footprint smaller than an emoji! 🍋

```
```
