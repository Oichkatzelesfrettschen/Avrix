# AVR Tool-chain Setup (Ubuntu 22.04 / 24.04 LTS)

This section replaces all earlier, partially-outdated snippets. It consolidates the entire discussion, the **setup.sh** script, and the latest Launchpad/Debian realities into one authoritative guide.

---

## 1 · Pick a compiler path

| Mode                     | GCC ver. | Where it lives                                            | Pros                                          | Cons                           |
| ------------------------ | -------- | --------------------------------------------------------- | --------------------------------------------- | ------------------------------ |
| **Modern (recommended)** | 14 .2    | *Debian sid* cross packages (pinned) **or** xPack tarball | Full C23, `-mrelax`, `-mcall-prologues` fixes | One extra `sources.list` entry |
| **Legacy**               | 7 .3     | Ubuntu *universe* (default)                               | Already in archive, rock-solid                | C11 only, larger binaries      |

> ❗ **No Launchpad PPA currently publishes an AVR cross GCC ≥ 10.**
> Old docs that mention `ppa:team-gcc-arm-embedded/avr` or
> `ppa:ubuntu-toolchain-r/test` for AVR should be ignored—they ship only *host*
> compilers.

---

## 2 · Modern route (A) — Debian-sid pin

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

You will get **gcc-avr 14.2.0-2** (May 2025 upload) plus the latest
`avr-libc 2.2`.

---

## 3 · Modern route (B) — xPack tarball (no root)

```bash
curl -L -o /tmp/avr.tgz \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
mkdir -p $HOME/opt/avr && tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile
```

Gives GCC 13.2 with identical C23 support (+ LTO, relax, prologue sharing).

---

## 4 · Legacy route — Ubuntu archive

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

Installs **gcc-avr 7.3.0+Atmel-3.7**.

---

## 5 · Common development helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm
pip3 install --user breathe exhale sphinx-rtd-theme
npm  install   -g    prettier
```

---

## 6 · One-shot bootstrap script

```bash
sudo ./setup.sh --modern        # or  --legacy  /  --build
```

* Detects running mode, adds Debian pin if needed.
* Installs compiler, QEMU, Static-analysis tools, Prettier.
* Prints MCU-specific **CFLAGS/LDFLAGS** (override via `MCU` / `F_CPU` env).

---

## 7 · Recommended optimisation flags (ATmega328P)

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

With GCC 14 you may add `--icf=safe -fipa-pta` for another \~2 % flash drop.

---

## 8 · Building with Meson

```bash
meson setup build --wipe \
      --cross-file cross/atmega328p_gcc14.cross    # ships in repo
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## 9 · Verifying install

```bash
avr-gcc       --version | head -1    # expect 13.x or 14.x
avr-libc      --version              # via dpkg-query -W avr-libc
qemu-system-avr --version | head -1
```

---

### Appendix · Lock-byte address override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "lock must be in lower I/O");
```

Override:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
     -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

---

All information above reflects the **current state as of 20 June 2025** and
matches the project’s `setup.sh`, Meson cross files, and CI pipeline.
