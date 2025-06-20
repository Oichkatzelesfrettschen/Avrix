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


After installation, verify the tool versions:

```bash
avr-gcc --version
dpkg-query -W -f 'avr-libc ${Version}\n' avr-libc

```

Optimised flags for an Arduino Uno (ATmega328P):

```bash
MCU=atmega328p
CFLAGS="-std=c11 -mmcu=$MCU -DF_CPU=16000000UL -Os -flto -ffunction-sections -fdata-sections"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

Note that the avr-gcc package provided by Ubuntu 24.04 is based on GCC 7 and
supports the C11 language standard.  The library code therefore targets C11
rather than newer dialects such as C23.


## Building the Library

The included `Makefile` builds `libavrix.a`, a collection of minimal
operating system primitives suitable for an ATmega328P.
Compile with:

```bash
make
```

If `avr-gcc` is not present, the build will fail. Run `setup.sh` or install the
toolchain manually before invoking `make`.

The recommended compiler flags (also used in the Makefile) are optimised for
flash size and performance. Ensure the AVR-GCC toolchain is installed by running
`setup.sh` as described above.
