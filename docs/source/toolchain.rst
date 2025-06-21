.. _toolchain-setup:

========================================================
AVR Tool-chain Setup (Ubuntu 22.04 / 24.04 «Noble Numbat»)
========================================================

µ-UNIX targets the **Arduino Uno R3**  
:- ATmega328P (32 k flash / 2 k SRAM)  
:- ATmega16U2 USB bridge (LUFA CDC-ACM)  

Two **fully-CI-tested** tool-chain paths are supported.  
Every Meson cross-file, script and example in the repo assumes **one** of
them. If you only care about “it just works”, run:

.. code-block:: bash

   sudo ./setup.sh --modern      # Debian gcc-avr 14 + QEMU demo
   sudo ./setup.sh --legacy      # Ubuntu gcc-avr 7.3 only

``setup.sh`` in *modern* mode will pin the Debian-sid packages, install
QEMU ≥ 8.2, Meson, Doxygen, Sphinx … compile a demo ELF, boot it in
QEMU and finally print copy-&-paste **C23** flags.  
*Legacy* mode installs only the Ubuntu packages and prints **C11**
flags — no QEMU build, no demo.

----------------------------------------------------------------------
1 · Select a compiler source
----------------------------------------------------------------------

+----------------+-----------+--------------------------------------+---------------------------------------------+
| Mode           | GCC ver.  | Where it lives                       | Pros / Cons                                 |
+================+===========+======================================+=============================================+
| **Modern**     | 14.2 (2025)| Debian-sid cross pkgs **or** xPack   | ✔ Full C23, `-mrelax`, smallest flash       |  
|                |           |                                      | ✘ Needs *pin* **or** PATH edit              |
+----------------+-----------+--------------------------------------+---------------------------------------------+
| **Legacy**     | 7.3 (2018)| Ubuntu *universe*                    | ✔ Already in archive, zero effort           |  
|                |           |                                      | ✘ C11 only, ≈ 8 % bigger binaries           |
+----------------+-----------+--------------------------------------+---------------------------------------------+

.. admonition:: No Launchpad PPA

   As of **June 2025** *no* Launchpad PPA provides AVR gcc ≥ 10.  
   Ignore guides that reference ``ppa:team-gcc-arm-embedded/avr`` or
   ``ppa:ubuntu-toolchain-r/test`` for AVR work.

----------------------------------------------------------------------
2 A · Modern path via Debian-sid pin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Add sid repo (signed) and pin exactly three packages
   echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
        http://deb.debian.org/debian sid main' \
        | sudo tee /etc/apt/sources.list.d/debian-sid-avr.list

   sudo tee /etc/apt/preferences.d/90-avr <<'EOF'
   Package: gcc-avr avr-libc binutils-avr
   Pin: release o=Debian,a=sid
   Pin-Priority: 100
   EOF

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc

*Current sid*: **gcc-avr 14.2.0-2**, **avr-libc 2.2**

----------------------------------------------------------------------
2 B · Modern path via xPack tarball (no root)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   curl -L -o /tmp/avr.tgz \
        https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
   mkdir -p "$HOME/opt/avr"
   tar -C "$HOME/opt/avr" --strip-components=1 -xf /tmp/avr.tgz
   echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile

Gives **GCC 13.2** (full C23 + ThinLTO) without touching APT.

----------------------------------------------------------------------
2 C · Legacy path (Ubuntu archive)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo apt update
   sudo apt install -y gcc-avr avr-libc binutils-avr \
                       avrdude gdb-avr qemu-system-misc   # gcc 7.3

----------------------------------------------------------------------
2 D · Workaround while you wait
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the Debian-sid cross packages are temporarily uninstallable on
Ubuntu **Noble**, rebuild them locally from source:

.. code-block:: bash

   sudo add-apt-repository deb-src http://ftp.debian.org/debian sid main
   sudo apt update
   apt source gcc-avr
   cd gcc-avr-14.2.0-2
   sudo apt build-dep .
   debuild -us -uc     # builds .deb for your host; install with dpkg -i

This ``pull-and-rebuild`` approach succeeds on Noble because its
``binutils-avr`` and ``avr-libc`` already satisfy 14.2’s build deps.
See `Ask Ubuntu <https://askubuntu.com/>`_ for background.

----------------------------------------------------------------------
3 · Developer helpers
----------------------------------------------------------------------

.. code-block:: bash

   sudo apt install -y meson ninja-build doxygen python3-sphinx simavr \
                      python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                      nodejs npm
   pip3 install --user breathe exhale sphinx-rtd-theme
   npm  install  -g   prettier

----------------------------------------------------------------------
4 · Sanity-check the install
----------------------------------------------------------------------

.. code-block:: bash

   avr-gcc --version        | head -1   # 14.x expected for modern path
   dpkg-query -W -f='avr-libc %V\n' avr-libc
   qemu-system-avr --version | head -1

----------------------------------------------------------------------
5 · Optimisation flags (Arduino Uno R3)
----------------------------------------------------------------------

.. code-block:: bash

   MCU=atmega328p
   CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
           -ffunction-sections -fdata-sections -mcall-prologues \
           --icf=safe -fipa-pta"
   LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

For a *legacy* build drop ``--icf`` / ``-fipa-pta`` and switch
``-std=c23`` → ``-std=c11``.

----------------------------------------------------------------------
6 · Building with Meson + QEMU
----------------------------------------------------------------------

.. code-block:: bash

   meson setup build --wipe \
        --cross-file cross/atmega328p_gcc14.cross -Dc_std=c23
   meson compile -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

``cross/atmega328p_clang20.cross`` is provided for LLVM 20 users.

----------------------------------------------------------------------
7 · Documentation targets
----------------------------------------------------------------------

.. code-block:: bash

   meson compile -C build doc-doxygen   # API reference
   meson compile -C build doc-sphinx    # user manual
   meson compile -C build doc           # sequential build; warnings fail

``dot`` from Graphviz is detected automatically to produce call graphs.

6 A · Docker image
------------------

Create an isolated environment via ``docker/Dockerfile``::

   docker build -t avrix-qemu docker
   docker run --rm -it avrix-qemu

The container builds the firmware, generates ``avrix.img`` and launches
``qemu-system-avr`` with the USB bridge enabled.

----------------------------------------------------------------------
8 · APT cheat-sheet
----------------------------------------------------------------------

Use these queries to see which versions your repositories provide
before running ``setup.sh``:

.. code-block:: bash

   apt-cache search  gcc-avr
   apt-cache show    gcc-avr-14 | grep ^Version
   apt-cache policy  gcc-avr              # view repo priorities
   apt-cache madison gcc-avr-14           # list all 14.x builds

