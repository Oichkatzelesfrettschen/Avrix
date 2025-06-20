# AVR Toolchain Setup

Run the script below to install the AVR-GCC toolchain on Ubuntu 24.04.
The script attempts to install the latest cross compiler available by
enabling the *ubuntu-toolchain-r/test* PPA and searching for
\`gcc-<version>-avr\` packages.  If none are found it falls back to the
stock \`gcc-avr\` from the \`universe\` repository.

```bash
sudo ./setup.sh            # installs the newest toolchain it can find
```

Pass `--stock` to force the script to use only Ubuntu's packages.
Use `--old` to try the deprecated pmjdebruijn PPA on older systems.

After installation, verify the toolchain versions:

```bash
avr-gcc --version
avr-libc-config --version
```

Optimised flags for an Arduino Uno (ATmega328P):

```bash
MCU=atmega328p
CFLAGS="-mmcu=$MCU -DF_CPU=16000000UL -Os -flto -ffunction-sections -fdata-sections"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

