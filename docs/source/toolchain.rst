.. _toolchain-setup:

===============================
AVR Tool-chain Setup (Ubuntu)
===============================

The project supports **two fully-tested paths** to a modern AVR
cross-compiler on Ubuntu 22.04 / 24.04.  Everything else in this
repository—including the Meson cross files and CI scripts—assumes that one
of these paths is active.

----------

Quick-start
-----------

.. code:: bash

   # one-shot, root-level bootstrap
   sudo ./setup.sh --modern     # or  --legacy  /  --build

``setup.sh``

* adds or skips the Debian-sid pin (modern)  
* installs avr-gcc, avr-libc, binutils, QEMU, analysis tools, Prettier  
* prints the MCU-specific **CFLAGS/LDFLAGS** to copy into other scripts.

The remainder of this file explains the manual route underpinning
`setup.sh`.

----------

1. Choose a compiler source
---------------------------

================  ==========  Where it lives                       Pros / Cons
Mode
================  ==========  ===================================  ======================================
**Modern**        14.2 (2025) Debian-sid cross packages **or**     + Full C23, `-mrelax`, tiny output  
                                 xPack binary tarball               – One extra `sources.list` entry
**Legacy**        7.3 (2018)  Ubuntu *universe*                     + Already in archive  
                                                                   – C11 only, ~8 % larger binaries
================  ==========  ===================================  ======================================

.. admonition:: No usable Launchpad PPA  
   As of June 2025 no Launchpad PPA publishes an AVR cross build
   newer than 7.3.  Old instructions mentioning
   ``ppa:team-gcc-arm-embedded/avr`` or
   ``ppa:ubuntu-toolchain-r/test`` for **AVR** are obsolete.

----------

2 A. Modern via Debian-sid pin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

*Current sid version:* **gcc-avr 14.2.0-2**, avr-libc 2.2.

2 B. Modern via xPack tarball (no root)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   curl -L -o /tmp/avr.tgz \
        https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz

   mkdir -p $HOME/opt/avr
   tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
   echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile
   source ~/.profile

Provides GCC 13.2 (C23-capable) without touching APT.

2 C. Legacy (Ubuntu archive)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc     # gcc 7.3

----------

3. Development helpers
----------------------

.. code:: bash

   sudo apt install -y meson ninja-build doxygen python3-sphinx \
                       python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                       nodejs npm
   pip3 install --user breathe exhale sphinx-rtd-theme
   npm  install   -g   prettier

----------

4. Sanity-check the tool-chain
------------------------------

.. code:: bash

   avr-gcc         --version | head -1   # 13.x or 14.x for modern
   avr-libc-config --version             # via dpkg-query on Ubuntu
   qemu-system-avr --version | head -1

----------

5. Optimisation flags (Arduino Uno R3)
--------------------------------------

.. code:: bash

   MCU=atmega328p
   CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
           -ffunction-sections -fdata-sections -mcall-prologues"
   LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

For GCC 14 add ``--icf=safe -fipa-pta`` for an extra ≈2 % flash saving.

----------

6. Building with Meson
----------------------

.. code:: bash

   meson setup build --wipe \
        --cross-file cross/atmega328p_gcc14.cross   # ships in repo
   meson compile -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

Documentation targets:

.. code:: bash

   meson compile -C build doc-doxygen
   meson compile -C build doc-sphinx

----------

7. Frequently used APT queries
------------------------------

.. code:: bash

   apt-cache search gcc-avr
   apt-cache show   gcc-avr | grep ^Version
   apt-cache policy gcc-avr        # shows all repos + chosen candidate
