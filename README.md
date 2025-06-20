# AVR Toolchain Setup

Run the script below to install the AVR-GCC toolchain on Ubuntu 24.04.
The script attempts to install the latest cross compiler available by
enabling the *ubuntu-toolchain-r/test* PPA and searching for
\`gcc-<version>-avr\` packages using `apt-cache search`. If none are found it
falls back to the stock \`gcc-avr\` from the \`universe\` repository. Recent
versions of the toolchain are also available from the `team-gcc-arm-embedded`
PPA which provides packages such as `gcc-avr-14`.

```bash
sudo ./setup.sh            # installs the newest toolchain it can find
```
This script installs the following packages:

- `gcc-avr` – GNU C cross compiler
- `avr-libc` – Standard C library for AVR development
- `binutils-avr` – assembler and binary utilities
- `avrdude` – firmware programmer
- `gdb-avr` – debugger
- `simavr` – lightweight simulator

To perform the installation manually, inspect the APT cache to see which
compiler packages the configured repositories provide.

`apt-cache search` lists packages matching a pattern, while
`apt-cache show` exposes the version and dependency information for a
particular package.  A truncated search confirms that GCC 14 is
available:

```bash
apt-cache search gcc-avr | head -n 2
gcc-avr - GNU C compiler for AVR microcontrollers
gcc-avr-14 - GNU C compiler for AVR microcontrollers (version 14)
apt-cache show gcc-avr-14 | grep ^Version
man apt-cache                # explore additional query options
```

`apt-cache policy` lists versions of a package across all configured
repositories and identifies which one is selected for installation.
Run it after refreshing the package lists to verify that the PPA offers a
newer compiler than Ubuntu's defaults:

```bash
sudo apt-get update         # ensure apt cache is current
apt-cache policy gcc-avr-14
```

Then install the desired tools with automatic confirmation:

```bash
sudo add-apt-repository ppa:team-gcc-arm-embedded/avr
sudo apt-get update
sudo apt-get install -y gcc-avr-14 avr-libc binutils-avr avrdude gdb-avr simavr
pip3 install --user meson
pip3 install --user breathe exhale
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
CFLAGS="-std=c11 -mmcu=$MCU -DF_CPU=16000000UL -Os -flto -ffunction-sections -fdata-sections"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```
Additional size savings can be gained with GCC 14:

```bash
CFLAGS="$CFLAGS --icf=safe -fipa-pta"
```
These options enable identical code folding and a reduced
points-to analysis for slightly smaller binaries.

Ubuntu 24.04 ships `gcc-avr` based on GCC 7.3.0 which only supports the C11
language standard.  For bleeding-edge features one may install
`gcc-avr-14` from the **team-gcc-arm-embedded** PPA.  The library builds
cleanly with either compiler but is written to remain compatible with C11.

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

```bash
meson setup build --wipe --cross-file cross/avr_m328p.txt
meson compile -C build
```

The resulting static library `libavrix.a` can be found in the build
directory.  Documentation is generated with:

```bash
meson compile -C build doc-doxygen    # fails if the target is disabled
meson compile -C build doc-sphinx     # fails when docs/doxygen/xml is missing
```
The HTML output is written to `build/docs` and integrates Doxygen
comments via the `breathe` and `exhale` extensions.

Use `setup.sh` or the manual commands above to install the compiler
before configuring Meson.
