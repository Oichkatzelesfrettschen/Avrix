Toolchain Setup
===============

The AVR development environment relies on a number of packages from the
Ubuntu archives.  Ubuntu 24.04 only ships ``gcc-avr`` version 7.3, so we
recommend enabling the toolchain test repository for a newer host compiler
and then installing the standard cross compiler:

.. code-block:: bash

   sudo add-apt-repository ppa:ubuntu-toolchain-r/test
   sudo apt-get update
   sudo apt-get install gcc-avr avr-libc binutils-avr avrdude gdb-avr simavr

``gcc-avr`` provides the GNU C cross compiler, while ``avr-libc``
contains the AVR C library and headers. ``binutils-avr`` supplies the
assembler and linker, ``avrdude`` programs flash memory, ``gdb-avr``
enables debugging and ``simavr`` offers a lightweight simulator.

Use ``apt-cache search gcc-avr`` to confirm which compiler packages are
available.  At present only ``gcc-avr`` 7.3 appears in the official
archive and the toolchain test repository, so the cross compiler remains
on the older C11-capable version.

Modern compiler options
-----------------------
If your project relies on the C23 standard you have several ways to obtain a newer AVR toolchain:

* **Debian sid packages** provide ``gcc-avr`` 14.2. Pin only ``gcc-avr``, ``avr-libc`` and ``binutils-avr`` while keeping the rest of Ubuntu.
* **xPack binaries** are pre-built archives. Unpack to ``/opt/avr`` and prepend that directory to ``PATH``.
* **Build from source** using ``--target=avr`` for maximum control.

Additional utilities useful for development and static analysis can be
installed with:

.. code-block:: bash

   sudo apt-get install meson ninja-build doxygen python3-sphinx \
        cloc cscope exuberant-ctags cppcheck graphviz

The documentation requires the ``breathe`` and ``exhale`` extensions
available on PyPI:

.. code-block:: bash

   pip3 install --user breathe exhale

Running the ``setup.sh`` script found in the project root installs these
packages automatically when executed with ``sudo``.
