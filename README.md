````markdown
# µ-UNIX for AVR  
*A ≤ 10 kB C23 nanokernel, wear-levelled log-FS, and lock/RPC suite for the Arduino Uno R3.*

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

> **Snapshot · 20 Jun 2025**  
> Every command below is exercised by CI against the current repo and the
> latest **`setup.sh`**.

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern           # GCC-14 tool-chain + QEMU smoke-boot
````

`setup.sh` will

* pin the **Debian-sid** cross packages (GCC 14) or transparently fall back to Ubuntu’s 7.3 tool-chain,
* install QEMU ≥ 8.2, Meson, Doxygen, Sphinx, graphviz, Prettier, etc.,
* **build** the firmware, boot it in QEMU (`arduino-uno` machine),
* print MCU-specific `CFLAGS`/`LDFLAGS` ready to paste into your Makefile.

Run `./setup.sh --help` to see all options or to perform the installation
steps manually.

---

## 1 · Compiler choices

| Mode                       | GCC      | Source                                         | ✅ Pros                                            | ⚠️ Cons                          |
| -------------------------- | -------- | ---------------------------------------------- | ------------------------------------------------- | -------------------------------- |
| **Modern** *(recommended)* | **14.2** | Debian-sid cross packages **or** xPack tarball | C23, `-mrelax`, `-mcall-prologues`, smallest code | needs an *apt* pin or PATH tweak |
| **Legacy**                 | 7.3      | Ubuntu *universe*                              | built-in, zero extra setup                        | C11 only, ≈ 8 % more flash       |

```bash
apt-cache search gcc-avr | grep -E '^gcc-avr-14\b'
gcc-avr-14 - GNU C compiler for AVR microcontrollers (version 14)
apt-cache show gcc-avr-14 | grep ^Version
```

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

### 1B · xPack tarball (modern, no root)

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
                    avrdude gdb-avr qemu-system-misc        # gcc 7.3
```

---

## 2 · Dev helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip python3-venv cloc cscope exuberant-ctags \
                    cppcheck graphviz nodejs npm
# Use a dedicated virtual environment for the Sphinx extensions
python3 -m venv ~/.avrix-docs
~/.avrix-docs/bin/pip install --upgrade breathe exhale sphinx-rtd-theme
npm  install  -g   prettier
```

---

## 3 · Recommended flags (ATmega328P)

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

# GCC-14 bonus
CFLAGS="$CFLAGS --icf=safe -fipa-pta"
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
avr-gcc         --version | head -1     # expect 14.2.x
dpkg-query -W -f='avr-libc %V\n' avr-libc
qemu-system-avr --version | head -1
```

---

## 6 · Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must live in lower I/O space");
```

Override during configuration:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

## 7 · Hardware target

| Chip           | Role    | Clock          | Flash / SRAM | Notes                |
| -------------- | ------- | -------------- | ------------ | -------------------- |
| **ATmega328P** | App MCU | 16 MHz crystal | 32 k / 2 k   | 8-bit AVRe+, Harvard |
| **ATmega16U2** | USB-CDC | 48 MHz PLL     | 16 k / 512 B | LUFA firmware        |

---

## 8 · What you get

* **Nanokernel** (< 10 kB) – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (420 B flash)
* **Door RPC** – zero-copy slab, \~1 µs latency
* **Spin-locks** – TAS / quaternion / lattice variants
* **Fixed-point Q8.8** helpers
* **Full QEMU board model** for CI

---

## 9 · Contributing

1. Fork & branch (`feat/short-title`).
2. Keep additions **tiny** – flash is precious.
3. `ninja -C build && meson test` must pass.
4. Update `docs/monograph.rst` with new flags or memory impact.

---

## 10 · Example: FS demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates two files in TinyLog-4, reads them back, prints via UART (view
with the QEMU serial console or a USB-UART dongle).

---

Run `./setup.sh` to install the toolchain or invoke `./setup.sh --help`
for a breakdown of the manual commands before configuring Meson.

## Performance checks with clang-tidy

The repository ships `optimize.sh`, a convenience wrapper around
``clang-tidy``. The script runs the ``performance-*`` checks over every
source file in ``src``. Execute it once ``clang-tidy`` is installed:

```bash
./optimize.sh
```

Extra options are forwarded to ``clang-tidy`` and the ``MCU``
environment variable selects the target AVR chip.

Happy hacking — the whole OS still fits in **less flash than one JPEG emoji** 🐜

```
```
