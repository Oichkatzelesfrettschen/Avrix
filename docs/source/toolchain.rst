Toolchain Setup
===============

The AVR development environment relies on a number of packages from the
Ubuntu archives. Install them with:

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

Recommended optimisation flags for the ATmega328P are::

   CFLAGS="-std=c23 -mmcu=atmega328p -DF_CPU=16000000UL -Oz -flto -mrelax \
       -ffunction-sections -fdata-sections -mcall-prologues"
   LDFLAGS="-mmcu=atmega328p -Wl,--gc-sections -flto"

The documentation requires the ``breathe`` and ``exhale`` extensions
available on PyPI:

.. code-block:: bash

   pip3 install --user breathe exhale

Running the ``setup.sh`` script found in the project root installs these
packages automatically when executed with ``sudo``. The script enables the
``team-gcc-arm-embedded/avr`` repository when necessary, refreshes the
package lists once, and verifies each required package with ``dpkg -s`` before
installation.
