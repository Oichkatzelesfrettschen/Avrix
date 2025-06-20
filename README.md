# AVR Toolchain Setup

Run the script below to install the AVR-GCC toolchain on Ubuntu 24.04.
By default it enables the `team-gcc-arm-embedded/avr` PPA and installs
`gcc-avr-14`, providing modern C23 support and the latest AVR
optimisations. Passing `--legacy` installs the stock `gcc-avr` from the
`universe` repository instead.

```bash
sudo ./setup.sh            # installs the newest toolchain it can find
```
The script verifies each package with `dpkg -s` and performs only one
`apt-get update` after the PPA is enabled. Supporting utilities are installed
on demand, avoiding redundant package operations.
This script installs the following packages:

- `gcc-avr` – GNU C cross compiler
- `avr-libc` – Standard C library for AVR development
- `binutils-avr` – assembler and binary utilities
- `avrdude` – firmware programmer
- `gdb-avr` – debugger
- `simavr` – lightweight simulator

To install them manually without the script first discover which GCC
packages are available using `apt-cache`:

```bash
apt-cache search gcc-avr
apt-cache show gcc-avr-14    # inspect package details
man apt-cache                # explore additional query options
```

Then install the desired tools:

```bash
sudo add-apt-repository ppa:team-gcc-arm-embedded/avr
sudo apt-get update
sudo apt-get install gcc-avr-14 avr-libc binutils-avr avrdude gdb-avr simavr
```
Legacy systems can instead install the stock `gcc-avr` (version 7.3.0) from the
Ubuntu archives.

Additional developer utilities are recommended for code analysis and
documentation generation.  Install them with:

```bash
sudo apt-get install meson ninja-build doxygen python3-sphinx \
     cloc cscope exuberant-ctags cppcheck graphviz
```

The Sphinx extensions `breathe` and `exhale` are distributed on PyPI:

```bash
pip3 install --user breathe exhale
```


Pass `--legacy` to `setup.sh` to use Ubuntu's packages instead of the modern
PPA.  Using `--modern` (the default) selects GCC 14 from
`ppa:team-gcc-arm-embedded/avr`.


After installation, verify the tool versions:

```bash
avr-gcc --version
dpkg-query -W -f 'avr-libc ${Version}\n' avr-libc

```

Optimised flags for an Arduino Uno (ATmega328P):

```bash
MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```
Additional size savings can be gained with GCC 14:

```bash
CFLAGS="$CFLAGS --icf=safe -fipa-pta"
```
These options enable identical code folding and a reduced
points-to analysis for slightly smaller binaries.

## Configurable Lock Address

The spinlock primitives can operate on a dedicated I/O register. The
address is set at compile time with the `NK_LOCK_ADDR` macro which
defaults to `0x2C`.

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "lock must be in lower I/O");
```

Override this setting by appending a compiler flag, e.g.

```bash
meson setup build --cross-file cross/avr_m328p.txt \
  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

Ubuntu 24.04 ships `gcc-avr` based on GCC 7.3.0 which only supports the C11
language standard.  Installing `gcc-avr-14` from the
**team-gcc-arm-embedded** PPA unlocks full C23 support.
The sources target the modern standard yet remain broadly compatible with
C11 compilers.

## Hardware Target: Arduino Uno R3

Avrix is designed around the **Arduino Uno R3**, which combines an
ATmega328P application processor and an ATmega16U2 USB interface.  The
Uno R3 exposes 32 KB of flash, 2 KB of SRAM and runs at 16 MHz from a
crystal oscillator.  The USB microcontroller provides a USB
serial interface at 48 MHz.  All compiler flags and memory layouts in
this repository target these specific constraints.


## Building the Library

The project uses **Meson** in combination with a cross file to build
the AVR binaries.  Two examples are supplied:

- `meson/avr_gcc_cross.txt` – minimal flags
- `cross/avr_m328p.txt` – full example with absolute tool paths
  including optimisation flags tuned for the ATmega328P

```bash
meson setup build --cross-file cross/avr_m328p.txt
meson compile -C build
```

The resulting static library `libavrix.a` can be found in the build
directory.  Documentation is generated with:

```bash
meson compile -C build doc-doxygen
meson compile -C build doc-sphinx
```
The HTML output is written to `build/docs` and integrates Doxygen
comments via the `breathe` and `exhale` extensions.

Use `setup.sh` or the manual commands above to install the compiler
before configuring Meson.
