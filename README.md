# AVR Toolchain Setup

Run the script below to install the AVR-GCC toolchain on Ubuntu 24.04.
The distribution currently provides only `gcc-avr` (version 7.3).  The script
enables the *ubuntu-toolchain-r/test* repository for a newer host compiler but
still installs the stock cross compiler.  Older instructions referenced the
`team-gcc-arm-embedded` PPA, however that archive no longer publishes AVR
packages and should be avoided.

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

To install them manually without the script first discover which GCC
packages are available using `apt-cache`:

```bash
apt-cache search gcc-avr
apt-cache show gcc-avr       # inspect package details
man apt-cache                # explore additional query options
```

For Ubuntu 24.04 this search typically yields only ``gcc-avr`` version
7.3.0 from the ``universe`` repository even when the Toolchain Test PPA
is enabled.  As of this writing no newer AVR cross compiler packages are
published on Launchpad, so the script installs ``gcc-avr`` by default.

Then install the desired tools:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-avr avr-libc binutils-avr avrdude gdb-avr simavr
```
This installs the official cross compiler along with a modern host GCC from the
toolchain test PPA.  If the PPA is unavailable simply omit the first line and
install the packages from Ubuntu's universe repository.

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


Pass `--legacy` to `setup.sh` to skip adding the toolchain test repository.
Both modes install the same `gcc-avr` package on Ubuntu 24.04, but `--modern`
also enables `ppa:ubuntu-toolchain-r/test` for a recent host compiler.


After installation, verify the tool versions:


```bash
avr-gcc --version
dpkg-query -W -f 'avr-libc ${Version}\n' avr-libc
```


Modern compiler options
-----------------------
Ubuntu currently packages only ``gcc-avr`` 7.3. Several alternatives exist if you require a C23-capable toolchain:

* **Debian sid cross packages** – Pin ``gcc-avr`` from the unstable repository to obtain GCC 14 while keeping the rest of the system on Ubuntu.
* **xPack pre-built binaries** – Download the xPack AVR-GCC tarball and prepend ``/opt/avr/bin`` to ``PATH``.
* **Build from source** – Clone the GCC repository and configure with ``--target=avr`` for complete control over optimisation options.

Installing GCC 14 from Debian sid
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The Debian archive hosts modern cross compilers suitable for Ubuntu systems.
Create the following ``/etc/apt`` entries and then run ``apt update``:

.. code-block:: bash

   # /etc/apt/sources.list.d/debian-sid-avr.list
   deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
       http://deb.debian.org/debian sid main

   # /etc/apt/preferences.d/90avr-toolchain
   Package: gcc-avr avr-libc binutils-avr
   Pin: release o=Debian,a=sid
   Pin-Priority: 100

   sudo apt update
   sudo apt install gcc-avr avr-libc binutils-avr

Installing xPack AVR-GCC
~~~~~~~~~~~~~~~~~~~~~~~~
Pre-built toolchains are published at `xpack.github.io <https://xpack.github.io>`_.
Download the archive for your architecture and unpack it under ``/opt/avr``:

.. code-block:: bash

   wget https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/
       v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
   sudo mkdir -p /opt/avr
   sudo tar -C /opt/avr --strip-components=1 \
        -xf xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
   export PATH=/opt/avr/bin:"$PATH"


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
language standard.  If newer AVR compilers become available via Launchpad you
may substitute them here, but the codebase remains compatible with the stock
tools shipped by Ubuntu.

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
