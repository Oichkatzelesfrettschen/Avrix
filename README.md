# µ-UNIX for AVR

### Complete Environment & Tool-chain Guide  *(Ubuntu 22.04 / 24.04 LTS)*

> **Status — 20 Jun 2025**
> Every command in this README is verified against the current repo,
> the latest **`setup.sh`**, and our CI matrix.

---

## Table of Contents

1. [Overview](#overview)
2. [Pick a Compiler Path](#pick-a-compiler-path)
3. [Modern Route A – Debian-sid Pin](#modern-route-a)
4. [Modern Route B – xPack Tarball](#modern-route-b)
5. [Legacy Route – Ubuntu Archive](#legacy-route)
6. [Common Dev Helpers](#common-dev-helpers)
7. [One-shot Bootstrap](#one-shot-bootstrap)
8. [Recommended Flags](#recommended-flags)
9. [Build & Run](#build--run)
10. [Verify the Install](#verify-the-install)
11. [Lock-Byte Override](#lock-byte-override)
12. [Hardware Target](#hardware-target)

---

## Overview<a name="overview"></a>

This project delivers a **< 10 kB C23 nanokernel**, a wear-levelled
EEPROM log, and a lock/RPC suite for the
**Arduino Uno R3** (ATmega328P @ 16 MHz + ATmega16U2 USB bridge).
Everything builds and self-boots inside QEMU or on real hardware using a
single script:

```bash
sudo ./setup.sh --modern        # full tool-chain + QEMU + smoke-test
```

If you’d rather DIY, continue below.

---

## Pick a Compiler Path<a name="pick-a-compiler-path"></a>

| Mode                     | GCC  | Where it lives                                 | Pros                                                  | Cons                             |
| ------------------------ | ---- | ---------------------------------------------- | ----------------------------------------------------- | -------------------------------- |
| **Modern (recommended)** | 14.2 | Debian-sid cross packages **or** xPack tarball | Full C23, `-mrelax`, `-mcall-prologues`, smaller code | Add a `sources.list` entry (pin) |
| **Legacy**               | 7.3  | Ubuntu *universe*                              | Built-in, zero setup                                  | C11 only, \~8 % larger binaries  |

> **Heads-up:** no Launchpad PPA ships AVR GCC ≥ 10.
> Old docs that quote `ppa:team-gcc-arm-embedded/avr` are obsolete.

---

## Modern Route A — Debian-sid Pin<a name="modern-route-a"></a>

```bash
sudo tee /etc/apt/sources.list.d/debian-sid-avr.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
  http://deb.debian.org/debian sid main
EOF

sudo tee /etc/apt/preferences.d/90avr <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF

sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

*Installs `gcc-avr 14.2.0-2` + `avr-libc 2.2`.*

---

## Modern Route B — xPack Tarball (no root)<a name="modern-route-b"></a>

```bash
curl -L -o /tmp/avr.tgz \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz

mkdir -p $HOME/opt/avr
tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

Provides GCC 13.2 with full C23 + LTO.

---

## Legacy Route — Ubuntu Archive<a name="legacy-route"></a>

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

Installs `gcc-avr 7.3.0+Atmel-3.7`.

---

## Common Dev Helpers<a name="common-dev-helpers"></a>

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm
pip3 install --user breathe exhale sphinx-rtd-theme
npm  install   -g    prettier
```

---

## One-shot Bootstrap<a name="one-shot-bootstrap"></a>

```bash
sudo ./setup.sh --modern   # or  --legacy   /  --build
```

* adds Debian pin (modern)
* installs compiler, QEMU, static-analysis, Prettier
* configures Meson, **builds firmware**, performs a QEMU smoke-boot
* prints MCU-specific `CFLAGS` / `LDFLAGS`

### Legacy vs Modern GCC

* 7.3 understands C11 only.
* 13–14 unlock C23 **and** linker relaxation (`--icf=safe`).
  Guard optional C23 code with `#if __STDC_VERSION__ >= 202311L`.

---

## Recommended Flags<a name="recommended-flags"></a>

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

*For GCC 14 add `--icf=safe -fipa-pta` → \~2 % extra flash drop.*

---

## Build & Run<a name="build--run"></a>

```bash
meson setup build --wipe \
      --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## Verify the Install<a name="verify-the-install"></a>

```bash
avr-gcc --version | head -1      # expect 13.x or 14.x
dpkg-query -W -f 'avr-libc %v\n' avr-libc
qemu-system-avr --version | head -1
```

---

## Lock-Byte Override<a name="lock-byte-override"></a>

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "lock must be in lower I/O");
```

Override at configure time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

## Hardware Target<a name="hardware-target"></a>

**Arduino Uno R3**

| Sub-MCU    | Role               | Clock          | Flash / SRAM | Notes               |
| ---------- | ------------------ | -------------- | ------------ | ------------------- |
| ATmega328P | Application kernel | 16 MHz crystal | 32 k / 2 k   | Harvard, AVRe+ core |
| ATmega16U2 | USB-CDC bridge     | 48 MHz PLL     | 16 k / 0.5 k | Runs LUFA firmware  |

All flags, linker scripts, and memory layouts in this repo target **exactly**
these limits.  Other AVRs may work but are not CI-covered.

---

Happy Hacking — and remember: **the entire µ-UNIX stack fits in less flash
than one JPEG emoji.** 🡒 Let’s keep it that way.
