.. _toolchain-setup:

=========================================
AVR Tool-chain Setup (Ubuntu 22.04 / 24.04)
=========================================

This repository builds a **C23 µ-UNIX** that boots on an Arduino Uno R3
(**ATmega328P + ATmega16U2**) or in QEMU.  
Two tool-chain paths are fully CI-tested; every Meson cross-file, script and
markdown example assumes one of them.

*If you just want it to work, run **`sudo ./setup.sh --modern`** and skip the
rest of this page.*

----------------------------------------------------------------------
Quick-start
----------------------------------------------------------------------

.. code:: bash

   # one-shot, root-level bootstrap
   sudo ./setup.sh --modern      # or  --legacy   /  --build

``setup.sh`` will

* add or skip the **Debian-sid pin** (modern)  
* install avr-gcc, avr-libc, binutils, QEMU, analysis tools, Prettier  
* build a demo ELF, boot it in QEMU, print MCU-specific **CFLAGS/LDFLAGS**

The rest of this document explains the same steps manually.

----------------------------------------------------------------------
1 · Choose a compiler source
----------------------------------------------------------------------

================  ==========  Where it lives                     Pros / Cons
Mode
================  ==========  =================================  =========================================
**Modern**        14.2 (2025) Debian-sid cross pkg **or**        + Full C23, `-mrelax`, small binaries  
                                xPack tarball                     – Needs sid pin *or* PATH edit
**Legacy**        7.3 (2018)  Ubuntu *universe*                  + Already in archive, zero setup  
                                                                  – C11 only, ~8 % larger binaries
================  ==========  =================================  =========================================

.. admonition:: No usable Launchpad PPA  
   As of **June 2025** no Launchpad PPA publishes an AVR cross GCC ≥ 10.  
   Old advice referencing ``ppa:team-gcc-arm-embedded/avr`` or
   ``ppa:ubuntu-toolchain-r/test`` for AVR is obsolete.

----------------------------------------------------------------------
2 A · Modern via Debian-sid pin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   # add sid repo + low-priority pin (only for three packages)
   echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
        http://deb.debian.org/debian sid main' \
        | sudo tee /etc/apt/sources.list.d/debian-sid-avr.list

   sudo tee /etc/apt/preferences.d/90avr <<'EOF'
   Package: gcc-avr avr-libc binutils-avr
   Pin: release o=Debian,a=sid
   Pin-Priority: 100
   EOF

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc

*Current sid*: **gcc-avr 14.2.0-2**, avr-libc 2.2.

2 B · Modern via xPack tarball (no root)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   curl -L -o /tmp/avr.tgz \
        https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
   mkdir -p $HOME/opt/avr
   tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
   echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile
   source ~/.profile

Provides **GCC 13.2** (full C23, LTO) without touching APT.

2 C · Legacy (Ubuntu archive)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc      # gcc 7.3

----------------------------------------------------------------------
3 · Development helpers
----------------------------------------------------------------------

.. code:: bash

   sudo apt install -y meson ninja-build doxygen python3-sphinx \
                       python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                       nodejs npm
   pip3 install --user breathe exhale sphinx-rtd-theme
   npm  install   -g   prettier

----------------------------------------------------------------------
4 · Sanity-check the tool-chain
----------------------------------------------------------------------

.. code:: bash

   avr-gcc --version        | head -1      # 13.x or 14.x for modern
   dpkg-query -W avr-libc   | cut -f2
   qemu-system-avr --version| head -1

----------------------------------------------------------------------
5 · Optimisation flags (Uno R3)
----------------------------------------------------------------------

.. code:: bash

   MCU=atmega328p
   CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
           -ffunction-sections -fdata-sections -mcall-prologues"
   LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

*GCC 14 bonus*: add ``--icf=safe -fipa-pta`` for ≈ 2 % extra flash drop.

----------------------------------------------------------------------
6 · Building with Meson
----------------------------------------------------------------------

.. code:: bash

   meson setup build --wipe \
        --cross-file cross/atmega328p_gcc14.cross   # file is in repo
   meson compile -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

Docs:

.. code:: bash

   meson compile -C build doc-doxygen
   meson compile -C build doc-sphinx

----------------------------------------------------------------------
7 · Frequently used APT queries
----------------------------------------------------------------------

.. code:: bash

   apt-cache search  gcc-avr
   apt-cache show    gcc-avr | grep ^Version
   apt-cache policy  gcc-avr        # displays repo priorities
