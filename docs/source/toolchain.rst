.. _toolchain-setup:

=========================================
AVR Tool-chain Setup (Ubuntu 22.04 / 24.04)
=========================================

This repository builds a **C23 µ-UNIX** that boots on an Arduino Uno R3
(**ATmega328P + ATmega16U2**) or in QEMU.

Only **two** tool-chain paths are CI-tested; every Meson cross-file,
script and example in the repo assumes that *one* of them is active.

*If you just need it to work run either mode:*

.. code-block:: bash

   sudo ./setup.sh --modern      # Debian GCC‑14 + QEMU demo
   sudo ./setup.sh --legacy      # Ubuntu gcc-avr 7.3 only

----------------------------------------------------------------------
Quick-start
----------------------------------------------------------------------

``setup.sh`` provides two modes

* ``--modern`` pins the **Debian-sid** packages for ``gcc-avr-14`` and installs
  QEMU, Meson, Doxygen, Sphinx, graphviz and Prettier.  It then compiles a demo
  ELF, boots it in QEMU and prints **C23** flags for your Makefile.
* ``--legacy`` uses Ubuntu’s ``gcc-avr`` package only, skipping the QEMU build
  and Meson demo.  The suggested flags instead target **C11**.

----------------------------------------------------------------------
1 · Choose a compiler source
----------------------------------------------------------------------

================  ==========  Where it lives                        Pros / Cons
Mode
================  ==========  ====================================  ======================================
**Modern**        14.2 (2025) Debian-sid cross pkgs **or**           + Full C23, `-mrelax`, tiny code  
                               xPack tarball                          − Needs a *pin* **or** PATH edit
**Legacy**        7.3 (2018)  Ubuntu *universe*                      + Already in archive, zero effort  
                                                                     − C11 only, ≈ 8 % larger binaries
================  ==========  ====================================  ======================================

.. admonition:: No Launchpad PPA

   As of **June 2025** *no* Launchpad PPA publishes an AVR cross
   GCC ≥ 10.  Old guides that suggest
   ``ppa:team-gcc-arm-embedded/avr`` or
   ``ppa:ubuntu-toolchain-r/test`` for **AVR** are obsolete.

----------------------------------------------------------------------
2 A · Modern via Debian-sid pin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Add sid repo (signed) and pin only 3 packages.
   echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
        http://deb.debian.org/debian sid main' | \
        sudo tee /etc/apt/sources.list.d/debian-sid-avr.list

   sudo tee /etc/apt/preferences.d/90avr <<'EOF'
   Package: gcc-avr avr-libc binutils-avr
   Pin: release o=Debian,a=sid
   Pin-Priority: 100
   EOF

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc

*Current sid*: **gcc-avr 14.2.0-2** · **avr-libc 2.2**

2 B · Modern via xPack tarball (no root)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   curl -L -o /tmp/avr.tgz \
        https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
   mkdir -p $HOME/opt/avr
   tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
   echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile

Provides **GCC 13.2** (full C23 + LTO) without touching APT.

2 C · Legacy (Ubuntu archive)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc   # gcc 7.3

----------------------------------------------------------------------
3 · Development helpers
----------------------------------------------------------------------

.. code-block:: bash

   sudo apt install -y meson ninja-build doxygen python3-sphinx simavr \
                      python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                      nodejs npm
   pip3 install --user breathe exhale sphinx-rtd-theme
   npm  install  -g    prettier

----------------------------------------------------------------------
4 · Sanity-check the install
----------------------------------------------------------------------

.. code-block:: bash

   avr-gcc --version        | head -1   # 14.x expected for modern path
   dpkg-query -W -f='avr-libc %V\n' avr-libc
   qemu-system-avr --version | head -1

----------------------------------------------------------------------
5 · Optimisation flags (Uno R3)
----------------------------------------------------------------------

.. code-block:: bash

   MCU=atmega328p
   CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
           -ffunction-sections -fdata-sections -mcall-prologues"
   LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

   # GCC 14 bonus
   CFLAGS="$CFLAGS --icf=safe -fipa-pta"

----------------------------------------------------------------------
6 · Building with Meson
----------------------------------------------------------------------

.. code-block:: bash

   meson setup build --wipe \
        --cross-file cross/atmega328p_gcc14.cross
   meson compile -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

Documentation targets:

.. code-block:: bash

   meson compile -C build doc-doxygen
   meson compile -C build doc-sphinx

----------------------------------------------------------------------
7 · Handy APT queries
----------------------------------------------------------------------

.. code-block:: bash

   apt-cache search  gcc-avr
   apt-cache show    gcc-avr-14 | grep ^Version
   apt-cache policy  gcc-avr               # see repo priorities
