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

Legacy vs Modern GCC
--------------------

``gcc-avr`` in Ubuntu 24.04 is version 7.3 and supports the C11 standard.
The ``team-gcc-arm-embedded`` PPA provides ``gcc-avr-14`` with full C23
support, improved link-time optimisation and additional size-reduction
flags like ``--icf=safe`` and ``-fipa-pta``.  The project builds with
either compiler, but code relying on new C23 features must be conditionally
compiled when using the legacy toolchain.

Switch the compiler during setup by passing ``--modern`` (default) or
``--legacy`` to ``setup.sh``:

.. code-block:: bash

   sudo ./setup.sh --modern   # install GCC 14
   sudo ./setup.sh --legacy   # install GCC 7.3
