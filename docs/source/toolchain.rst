Toolchain Setup
===============

The AVR development environment relies on a number of packages from the
Ubuntu archives. Install them with:

.. code-block:: bash

   sudo add-apt-repository ppa:team-gcc-arm-embedded/avr
   sudo apt-get update
   sudo apt-get install -y gcc-avr-14 avr-libc binutils-avr avrdude gdb-avr simavr

Python tooling (requires ``pip3`` or a virtual environment)::

   pip3 install --user meson breathe exhale

``gcc-avr-14`` provides the GNU C cross compiler, while ``avr-libc``
contains the AVR C library and headers. ``binutils-avr`` supplies the
assembler and linker, ``avrdude`` programs flash memory, ``gdb-avr``
enables debugging and ``simavr`` offers a lightweight simulator.

Before installation you can query the repositories to determine which
compiler versions are available.  ``apt-cache search`` lists packages
matching a pattern, while ``apt-cache show`` prints version details for a
specific package.  A short search reveals GCC 14:

.. code-block:: bash

   apt-cache search gcc-avr | grep -E '^gcc-avr-14\\b'
   gcc-avr-14 - GNU C compiler for AVR microcontrollers (version 14)
   apt-cache show gcc-avr-14 | grep ^Version

``apt-cache policy`` lists the candidate package and shows which repository
contributes each available version.  Running it after ``sudo apt-get update``
confirms that the PPA indeed provides a newer compiler than Ubuntu's
defaults:

.. code-block:: bash

   sudo apt-get update       # refresh package lists
   apt-cache policy gcc-avr-14

Additional utilities useful for development and static analysis can be
installed with:

.. code-block:: bash

   sudo apt-get install meson ninja-build doxygen python3-sphinx \
        python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
        nodejs npm

JavaScript tools such as ``prettier`` can then be installed globally:

.. code-block:: bash

   npm install -g prettier

Recommended optimisation flags for the ATmega328P are::

   CFLAGS="-std=c23 -mmcu=atmega328p -DF_CPU=16000000UL -Oz -flto -mrelax \
       -ffunction-sections -fdata-sections -mcall-prologues"
   LDFLAGS="-mmcu=atmega328p -Wl,--gc-sections -flto"

The documentation requires the ``breathe`` and ``exhale`` extensions
available on PyPI:

.. code-block:: bash

   pip3 install --user breathe exhale sphinx-rtd-theme

Running the ``setup.sh`` script found in the project root installs these
packages automatically when executed with ``sudo``.

Building the sources with ``meson`` requires a clean build directory.  The
cross file ``cross/avr_m328p.txt`` configures the correct MCU and toolchain
paths:

.. code-block:: bash

   meson setup build --wipe --cross-file cross/avr_m328p.txt
   meson compile -C build
   meson compile -C build doc-doxygen    # fails if the target was skipped
   meson compile -C build doc-sphinx     # fails when Doxygen XML is missing

``--wipe`` deletes any existing ``build`` directory.  Use a dedicated
directory or back up artifacts you intend to keep.

The ``setup.sh`` helper installs these packages automatically.  Pass
``--modern`` or ``--legacy`` to choose the GCC source and set ``MCU`` and
``F_CPU`` to tailor the optimisation flags printed at the end of the run.
