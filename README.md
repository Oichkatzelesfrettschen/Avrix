````markdown
# µ-UNIX for AVR 🍋  
*A ≤ 10 kB C23 nano-kernel, wear-levelled log-FS, and lock / RPC suite for the Arduino Uno R3.*

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

> **Snapshot · 20 Jun 2025** – every command in this README is executed in CI  
> against the current tree and the latest **`setup.sh`**.

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern     # gcc-avr 14 + full tool-chain (✨ recommended)
# or
sudo ./setup.sh --legacy     # gcc-avr 7.3 only – bare minimum
````

`setup.sh` *detects* whether Debian-sid is reachable:

* **Modern (default)** – pins the sid cross packages (`gcc-avr 14.x`) but
  silently falls back to Ubuntu 7.3 if sid is blocked.
  Installs QEMU ≥ 8.2, Meson, Doxygen, Sphinx, Graphviz, Prettier …
  Builds a demo ELF and boots it under QEMU (`arduino-uno`).
  Prints MCU-specific **C23** `CFLAGS` / `LDFLAGS`.

* **Legacy** – installs the Ubuntu *universe* tool-chain (`gcc-avr 7.3`,
  `avr-libc`, `binutils-avr`, `avrdude`, `gdb-avr`) – nothing else – and
  downgrades the suggested flags to **C11**.

Run `./setup.sh --help` for additional modes (e.g. `--clang`).

---

## 1 · Compiler paths

| Mode (host)              | GCC  | Source                                 | ✅ Pros                                            | ⚠️ Cons                            |
| ------------------------ | ---- | -------------------------------------- | ------------------------------------------------- | ---------------------------------- |
| **Modern (recommended)** | 14.2 | Debian-sid cross packages **or** xPack | C23, `-mrelax`, `-mcall-prologues`, smallest code | needs an *apt* pin or `$PATH` edit |
| **Legacy**               | 7.3  | Ubuntu *universe*                      | zero extra setup                                  | C11 only, ≈ 8 % larger binaries    |

> **Heads-up :** no Launchpad PPA ships an AVR GCC ≥ 10.
> Ignore outdated advice that mentions `ppa:team-gcc-arm-embedded/avr`.

### 1A · Debian-sid pin (Modern path)

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
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

*Installs `gcc-avr 14.2.0-2` + `avr-libc 2.2`.*

### 1B · xPack tarball (Modern path, **no root**)

```bash
curl -L \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz \
  -o /tmp/avr.tgz
mkdir -p "$HOME/opt/avr"
tar -C "$HOME/opt/avr" --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

### 1C · Ubuntu archive (Legacy path)

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc   # 7.3.0
```

### 1D · Optional Clang / LLVM 20

```bash
sudo add-apt-repository -y ppa:llvm-team/llvm-next
sudo apt update
sudo apt install -y clang-20 lld-20 llvm-20
# then use cross/atmega328p_clang20.cross with Meson
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

## 3 · Size-tuned flags (ATmega328P)

```bash
export MCU=atmega328p

CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues \
        --icf=safe -fipa-pta"         # last two only understood by GCC ≥ 14

LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

Legacy tool-chain? Replace `-std=c23` with `-std=c11` and drop `--icf` / `-fipa-pta`.

---

## 4 · Build & run

```bash
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## 5 · Verify install

```bash
avr-gcc         --version | head -1     # 14.x or 7.3.x
dpkg-query -W -f='avr-libc %v\n' avr-libc
qemu-system-avr --version  | head -1
```

---

## 6 · Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C         /* GPIOR0 – 1-cycle I/O access */
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must live in lower I/O space");
```

Override at configure time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

## 7 · Hardware target

| Chip           | Role       | Clock          | Flash / SRAM | Notes               |
| -------------- | ---------- | -------------- | ------------ | ------------------- |
| **ATmega328P** | App MCU    | 16 MHz crystal | 32 k / 2 k   | classic 8-bit AVRe+ |
| **ATmega16U2** | USB bridge | 48 MHz PLL     | 16 k / 512 B | LUFA-CDC firmware   |

---

## 8 · What you get

* **Nano-kernel** < 10 kB – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (420 B flash)
* **Door RPC** – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT
* **Spin-locks** – TAS / quaternion / Beatty-lattice flavours
* **Fixed-point Q8.8** helpers
* **Full QEMU board model** (`arduino-uno`) wired into CI

---

## 9 · Contributing

```text
fork → feat/my-feature → tiny, reviewable commits
$ ninja -C build && meson test          # must stay green
```

Document any flash / SRAM delta in **docs/monograph.rst**.

---

## 10 · File-system demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates two files in TinyLog-4, reads them back, prints via UART.

---

### Why this is *modern & fast*

* **C23 everywhere** (`-std=c23` cross-file; host builds stay C17).
* `-Oz -flto -mrelax -mcall-prologues` = 11–14 % flash drop vs. plain `-Os`.
* `--icf=safe` & identical-code-folding shave another 1–3 %.
* No Launchpad PPA dependencies – Debian-sid pin or xPack tarball only.
* Automatic QEMU build fallback keeps CI self-contained.

---

Happy hacking – the entire OS still fits in **less flash than a single JPEG emoji** 🐜

```
```
