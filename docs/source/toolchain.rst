Toolchain Setup
===============

The AVR development environment relies on a number of packages from the
Ubuntu archives.  Install them with:

.. code-block:: bash

   sudo add-apt-repository ppa:team-gcc-arm-embedded/avr
   sudo apt-get update
   sudo apt-get install apt-transport-https software-properties-common \
        gcc-avr-14 avr-libc binutils-avr avrdude gdb-avr simavr

``gcc-avr-14`` provides the GNU C cross compiler, while ``avr-libc``
contains the AVR C library and headers. ``binutils-avr`` supplies the
assembler and linker, ``avrdude`` programs flash memory, ``gdb-avr``
enables debugging and ``simavr`` offers a lightweight simulator.

Additional utilities useful for development and static analysis can be
installed with:

.. code-block:: bash

   sudo apt-get install ninja-build doxygen python3-sphinx \
        cloc cscope exuberant-ctags cppcheck graphviz python3-pip

The latest Meson release as well as the ``breathe`` and ``exhale``
extensions are available on PyPI:

.. code-block:: bash

   pip3 install --user meson breathe exhale

Running the ``setup.sh`` script found in the project root installs these
packages automatically when executed with ``sudo``.  The script uses
``apt`` to fetch the toolchain and ``pip`` to install Meson and the
Sphinx extensions.

Configuring Meson
-----------------

With the toolchain installed, create a build directory using the
provided cross file.  It sets ``F_CPU`` and enables link-time
optimization for smaller binaries:

.. code-block:: bash

   meson setup build --cross-file cross/avr_m328p.txt
   meson compile -C build
