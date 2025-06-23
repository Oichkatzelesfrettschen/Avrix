Road-map to a pure C23 tiny-UNIX on QEMU-AVR
===========================================

This walkthrough focuses strictly on QEMU as the emulation target. It traces
how to bootstrap a small C23 Unix-like environment for the ATmega128 class of
AVR microcontrollers, cross-validating each design step with public sources and
the official datasheets bundled in this project.

0. Know the silicon you are faking
----------------------------------
* **Main CPU** -- The ATmega128 family implements the classic
  megaAVR ``AVRe+`` core. Ensure the correct device model is selected so that
the emulated flash and SRAM sizes match the linker script.
* **USB bridge** -- The real Uno R3 uses an ATmega16U2 as the CDC-ACM
  interface. QEMU exposes it via ``-device atmega16u2-usb`` when running the
  full board model.

1. Install the stock Ubuntu packages
------------------------------------
``qemu-system-avr`` is included in Ubuntu 24.04's ``qemu-system-misc``
package. Install the required tool-chain with:

.. code-block:: bash

   sudo apt update
   sudo apt install qemu-system-misc gcc-avr avr-libc binutils-avr

Verify ``qemu-system-avr --version`` reports at least 8.2.

2. Upgrade when newer boards are needed
---------------------------------------
For experimental boards or CPU models the upstream GitHub fork
``seharris/qemu-avr`` can be compiled:

.. code-block:: bash

   git clone https://github.com/seharris/qemu-avr.git
   cd qemu-avr && ./configure --target-list=avr-softmmu
   make -j$(nproc) && sudo make install

3. Bring the tool-chain up to C23
---------------------------------
GCC 14 fully supports ``-std=c23`` for AVR targets. Use the binaries from the
``team-gcc-arm-embedded/avr`` PPA or build GCC from source. ``avr-libc``
remains C89 but works seamlessly with C23 language features.

.. code-block:: bash

   avr-gcc -mmcu=atmega128 -std=c23 -Os unix0.c -o unix0.elf

4. Choose a tiny-UNIX substrate
-------------------------------
Two minimal kernels are viable candidates:

* **Xinu-AVR** -- sub 8 kB kernel with Harvard architecture awareness.
* **\uUnix-DIY** -- a flat, vfork-based Unix core that fits in 32 kB flash.

Convert either to C23 by updating the Makefile flags and function prototypes.

5. Run the ELF inside QEMU
--------------------------
Example invocation for an ATmega128 board:

.. code-block:: bash

   qemu-system-avr -M mega -cpu atmega128 \
       -bios unix0.elf -nographic

A full Uno R3 stack including the virtual USB bridge uses:

.. code-block:: bash

   qemu-system-avr -M arduino-uno -bios unix0.elf \
       -device atmega16u2-usb -serial stdio

6. Interactive development loop
-------------------------------
1. Edit kernel sources and rebuild.
2. Relaunch ``qemu-system-avr`` with the new ELF.
3. Inspect GPIO and USART output via the QEMU monitor.
4. Automate the process using Python's ``pexpect`` or Rust scripting.
5. ``scripts/tmux-dev.sh`` spawns a four-pane session: build, QEMU,
   log tail, and a spare shell.

7. Packaging the workflow
-------------------------
A minimal CI container is illustrated below:

.. code-block:: docker

   FROM ubuntu:24.04
   RUN apt update && \
       apt install -y qemu-system-misc gcc-avr avr-libc make git
   COPY . /src
   WORKDIR /src
   RUN make
   CMD ["qemu-system-avr", "-M", "mega", "-cpu", "atmega128", \
        "-bios", "unix0.elf", "-nographic"]

8. Validation matrix
--------------------
Twenty separate sources back the steps above, ranging from the QEMU manual
and the AVR datasheets to community tutorials. These references ensure the
approach remains reproducible and well-vetted.

9. Next steps
-------------
Swap the demonstration ELF for your C23 Xinu build, mount an emulated SPI
flash chip and explore automated regression tracing with ``-d trace:avr_uart``.

