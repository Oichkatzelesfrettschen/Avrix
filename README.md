````markdown
# µ-UNIX for AVR 🍋  
*A ≤ 10 kB C23 nano-kernel, wear-levelled log-FS, and lock / RPC suite for the Arduino Uno R3.*

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

> **Snapshot · 20 Jun 2025** – every command below is executed in CI
> against the current tree and the latest **`setup.sh`** script.

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern         # GCC-14 + QEMU smoke-boot (default)
# or
sudo ./setup.sh --legacy         # GCC-7.3 only, no extras
````

`setup.sh` will

* **--modern**   · pin **Debian-sid** cross packages (`gcc-avr 14.x`) or silently fall back to Ubuntu 7.3
  · install QEMU ≥ 8.2, Meson, Doxygen, Sphinx, Graphviz, Prettier …
  · **build** the firmware and boot it inside QEMU (`arduino-uno` model)
  · print MCU-specific `CFLAGS` / `LDFLAGS`.

* **--legacy**   · install Ubuntu’s `gcc-avr 7.3`, `avr-libc`, `binutils-avr`, `avrdude`, `gdb-avr` – nothing else.
  · suggested flags downgrade to **C11**.

See `./setup.sh --help` for advanced modes (`--clang`, `--deb-sid`, …).

---

## 1 · Compiler paths

| Mode                     | GCC  | Source                                  | ✅ Pros                                             | ⚠️ Cons                             |
| ------------------------ | ---- | --------------------------------------- | -------------------------------------------------- | ----------------------------------- |
| **Modern (recommended)** | 14.2 | Debian-sid cross pkgs **or** xPack 13.2 | C23, `-mrelax`, `-mcall-prologues`, smallest flash | needs an `apt` pin or `$PATH` tweak |
| **Legacy**               | 7.3  | Ubuntu *universe*                       | built-in, zero extra setup                         | C11 only, ≈ 8 % larger binaries     |

> **Heads-up:** no Launchpad PPA ships AVR GCC ≥ 10.
> Ignore old references to `ppa:team-gcc-arm-embedded/avr` or `ppa:ubuntu-toolchain-r/test`.

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
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

*Installs `gcc-avr 14.2.0-2` + `avr-libc 2.2`.*

### 1B · xPack tarball (Modern, no root)

```bash
curl -L -o /tmp/avr.tgz \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
mkdir -p "$HOME/opt/avr"
tar -C "$HOME/opt/avr" --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

### 1C · Ubuntu archive (Legacy)

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc     # 7.3.0
```

### 1D · Clang / LLVM 20 (optional)

```bash
sudo add-apt-repository ppa:llvm-team/llvm-next -y
sudo apt update
sudo apt install -y clang-20 lld-20 llvm-20
# use cross/atmega328p_clang20.cross with Meson
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
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

# GCC-14 bonus
CFLAGS+=" --icf=safe -fipa-pta"
```

---

## 4 · Build & run

```bash
meson setup build --wipe \
      --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## 5 · Verify install

```bash
avr-gcc         --version | head -1
dpkg-query -W -f='avr-libc %V\n' avr-libc
qemu-system-avr --version  | head -1
```

---

## 6 · Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must be in lower I/O space");
```

Override at configure time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

## 7 · Hardware target

| Chip           | Role            | Clock          | Flash / SRAM | Notes            |
| -------------- | --------------- | -------------- | ------------ | ---------------- |
| **ATmega328P** | Application MCU | 16 MHz crystal | 32 k / 2 k   | 8-bit AVRe+ core |
| **ATmega16U2** | USB bridge      | 48 MHz PLL     | 16 k / 512 B | LUFA CDC-ACM     |

---

## 8 · What you get

* **Nanokernel** < 10 kB – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (420 B flash)
* **Door RPC** – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT
* **Spin-locks** – TAS / quaternion / lattice variants
* **Fixed-point Q8.8** helpers
* **QEMU board model** (`arduino-uno`) wired into CI

---

## 9 · Contributing

```text
1.  fork → feat/my-feature
2.  keep patches tiny – flash is precious
3.  ninja -C build && meson test   # must stay green
4.  document flash / SRAM delta in docs/monograph.rst
```

---

## 10 · File-system demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates two files in TinyLog-4, then reads them back and prints over UART.

---

Happy hacking – the whole OS still fits in **less flash than a single JPEG emoji** 🐜

```
```
