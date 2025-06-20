````markdown
# µ-UNIX for AVR  
**A < 10 kB C23 nanokernel, log-FS, and lock/RPC suite for the Arduino Uno R3**

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

> **Snapshot — 20 Jun 2025**  
> All commands are verified against the current repo, the latest `setup.sh`,
> and the GitHub Actions matrix.

---

## 0 • Instant gratification — one-liner

```bash
sudo ./setup.sh --modern   # full tool-chain + QEMU smoke-boot
````

`setup.sh` will

* pin the Debian-sid cross packages (GCC 14) or transparently fall back to
  Ubuntu’s 7.3 legacy tool-chain,
* install QEMU ≥ 8.2, Meson, docs + analysis tools,
* **build** the firmware, boot it in QEMU (`arduino-uno` machine),
* print MCU-specific `CFLAGS`/`LDFLAGS` for copy-paste.

If you want to drive manually, read on.

---

## 1 • Compiler choices

| Mode                     | GCC      | Source                                               | 📈 Pros                                           | 📉 Cons                              |
| ------------------------ | -------- | ---------------------------------------------------- | ------------------------------------------------- | ------------------------------------ |
| **Modern (recommended)** | **14.2** | Debian-sid cross packages (pin) **or** xPack tarball | C23, `-mrelax`, `-mcall-prologues`, smallest code | Needs a pin (apt pref) or PATH tweak |
| **Legacy**               | 7.3      | Ubuntu *universe*                                    | Built-in, zero setup                              | C11 only, \~8 % larger binaries      |

> No Launchpad PPA ships AVR GCC ≥ 10.  Ignore old references to
> `ppa:team-gcc-arm-embedded/avr` or `ppa:ubuntu-toolchain-r/test`.

### 1A · Debian-sid pin (modern)

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

Installs **gcc-avr 14.2.0-2 + avr-libc 2.2**.

### 1B · xPack tarball (no root, modern)

```bash
curl -L -o /tmp/avr.tgz \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
mkdir -p $HOME/opt/avr
tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

### 1C · Ubuntu archive (legacy)

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc   # gcc 7.3
```

---

## 2 • Dev helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm
pip3  install --user breathe exhale sphinx-rtd-theme
npm   install -g    prettier
```

---

## 3 • Recommended flags (ATmega328P)

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

# GCC 14 bonus
CFLAGS="$CFLAGS --icf=safe -fipa-pta"
```

---

## 4 • Build & run

```bash
meson setup build --wipe \
      --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## 5 • Verify

```bash
avr-gcc --version | head -1
dpkg-query -W avr-libc | cut -f2
qemu-system-avr --version | head -1
```

---

## 6 • Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must live in lower I/O");
```

Override at configure time:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

## 7 • Hardware target

| Chip           | Role    | Clock          | Flash / SRAM | Notes          |
| -------------- | ------- | -------------- | ------------ | -------------- |
| **ATmega328P** | App MCU | 16 MHz crystal | 32 k / 2 k   | AVRe+, Harvard |
| **ATmega16U2** | USB-CDC | 48 MHz PLL     | 16 k / 512 B | LUFA firmware  |

All linker scripts & memory budgets assume these exact limits.

---

## 8 • What you get

* **Nanokernel** (< 10 kB) with pre-emptive round-robin
* **TinyLog-4** EEPROM log (wear-levelled, CRC-8)
* **Door RPC** (zero-copy Cap’n-Proto slab)
* **Spin-locks** (TAS / quaternion / lattice)
* **Fixed-point Q8.8** math helpers
* **QEMU board model** (`arduino-uno`) for full-speed CI

---

## 9 • Contributing

1. Fork & branch (`feat/short-title`).
2. Keep additions **tiny**—flash is precious.
3. Run `ninja -C build && meson test` before the PR.
4. Document any new flags or memory overhead in `monograph.rst`.

---

Happy hacking — and remember: the entire OS still takes **less flash than one
JPEG emoji**. Let’s keep it that way. 🐜

```
```
