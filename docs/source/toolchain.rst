Toolchain Setup
===============

The AVR development environment relies on a number of packages from the
Ubuntu archives.  Install them with:

.. code-block:: bash

   sudo add-apt-repository ppa:team-gcc-arm-embedded/avr
   sudo apt-get update
   sudo apt-get install gcc-avr-14 avr-libc binutils-avr avrdude gdb-avr simavr

``gcc-avr-14`` provides the GNU C cross compiler, while ``avr-libc``
contains the AVR C library and headers. ``binutils-avr`` supplies the
assembler and linker, ``avrdude`` programs flash memory, ``gdb-avr``
enables debugging and ``simavr`` offers a lightweight simulator.

Running the ``setup.sh`` script found in the project root installs these
packages automatically when executed with ``sudo``.
