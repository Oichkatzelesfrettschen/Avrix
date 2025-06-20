# AVR Toolchain Setup

Run the script below to install Atmel's AVR-GCC toolchain on Ubuntu 24.04:

```bash
sudo ./setup.sh            # installs via the pmjdebruijn PPA
```

To use Ubuntu's official packages instead, pass `--official`.

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
