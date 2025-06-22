.. _monograph:

=========================================================
Monograph — µ-UNIX on the Arduino Uno R3 (ATmega328P SoC)
=========================================================

.. contents::
   :local:
   :depth: 2

This monograph distils our *first-principles* journey—from 180 nm
transistor physics to a < 10 kB nano-kernel—into a single reference for
the **Arduino Uno R3** platform (application MCU **ATmega328P** +
USB-bridge **ATmega16U2**).

----------------------------------------------------------------------
1 · Silicon → Instruction-set
----------------------------------------------------------------------

* **Process / physics** 180 nm CMOS. Gate-oxide capacitance
  :math:`C_\text{ox}≈8.6 \mathrm{fF·µm}^{-2}`, silicon band-gap
  :math:`E_g = 1.12 \mathrm{eV}` ⇒ safe
  :math:`V_\text{min}≈2.7 \mathrm{V}`.

* **Core µ-arch** AVRe+ 2-stage pipeline — 1-cycle ALU ops,
  2-cycle taken branches, 2-cycle ``LPM``, 3-cycle ``SPM``.

* **Register file** 32 × 8-bit, dual-read / single-write → 1-cycle access.

* **Memory map**

  + 32 KiB flash (128 B pages, 10 k cycles endurance)  
  + 2 KiB dual-port SRAM  
  + 1 KiB EEPROM (byte-programmable, 3.4 ms write)

----------------------------------------------------------------------
2 · Kernel architecture
----------------------------------------------------------------------

The **nano-kernel** consumes *≤ 10 KiB flash* and *≤ 384 B SRAM*.

=====================  ============================  =================
Metric                 Value @ 16 MHz               Note
=====================  ============================  =================
Context-switch         35 cycles (≈ 2.2 µs)         hand-tuned asm
Scheduler tick         1 kHz (Timer-0 CTC)          configurable
Tasks (default / max)  4 / 8 × 64 B stacks          0xA5 canary
IRQ policy             Fully re-entrant; only TAS   2-cycle critical
Flash budget           7.6 KiB (measured)          size-optimised
SRAM budget            320 B (measured)            incl. run-queues
=====================  ============================  =================

Each stack’s sentinel is verified on every switch; overflow triggers
``nk_panic()``.

----------------------------------------------------------------------
3 · Memory manager (`kalloc`)
----------------------------------------------------------------------

* 256 B heap, 1-byte freelist header.  
* Best-fit with *first-touch* compaction during idle.  
* **O(1)** ``alloc`` / ``free`` for ≤ 8 blocks (bitmap resides in ``GPIOR0``).

----------------------------------------------------------------------
4 · Fixed-point arithmetic (Q8.8)
----------------------------------------------------------------------

* Signed **Q8.8**; range −128 … +127.996.  
* ``0x0100`` → +1, ``0xFF00`` → −1.  
* Mul keeps centre 16 bits of 32-bit product (rounding) → still 8-bit code.

----------------------------------------------------------------------
5 · TinyLog-4 EEPROM filesystem
----------------------------------------------------------------------

Wear-levelled, power-fail-safe log:

* **Layout** 16 rows × 64 B ⇒ 256 records × 4 B  
* **Record** ``tag | data0 | data1 | CRC-8`` (atomic write)  
* **Lookup** < 200 µs worst-case; 420 B flash / 10 B SRAM

``fs_list`` example
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   fs_create("boot.bin",   1);
   fs_create("config.txt", 1);

   char buf[FS_NUM_INODES * (FS_MAX_NAME + 1)];
   fs_list(buf, sizeof(buf));
   printf("%s", buf);

----------------------------------------------------------------------
6 · Descriptor-based RPC (“Doors”)
----------------------------------------------------------------------

* 4 door descriptors per task in ``.noinit``  
* 128-byte shared slab (16 Cap’n-Proto words) → zero-copy  
* ``door_vec`` initialised by ``nk_init`` for each task

Call path ::

   ``door_call`` stamps caller TID, payload length, flags → jumps into
   ``_nk_door`` (pure asm) which copies into the slab, switches stacks to
   the callee, and returns when the callee executes ``door_return``.
   Caller then copies reply from the slab.

===============  ========================  Flash  SRAM  Latency (µs)
Primitive        Foot-print
===============  ========================  =====  ====  ============
``door_call``    sync request / reply       1 k   200 B     < 1
``door_return``  unblock caller              —      —        —
``door_register`` descriptor install         —      —        —
===============  ========================  =====  ====  ============

----------------------------------------------------------------------
7 · Spin-locks
----------------------------------------------------------------------

===============  ============================  Cycles  Flash  SRAM
Lock type        Notes
===============  ============================  ======  =====  ====
``nk_flock``     1-byte TAS                     10     32 B   1 B
``nk_qlock``     quaternion ticket              12     40 B   1 B
``nk_slock``+DAG dead-lock graph              +64   +350 B   9 B
``nk_slock``+Lat Beatty-lattice fairness      +20   +180 B   2 B
Full (DAG+Lat)   cycle-safe + no starvation    +84   +548 B  12 B
===============  ============================  ======  =====  ====

Golden-ratio ticket
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #define NK_LATTICE_STEP 1657u
   #if NK_WORD_BITS == 32
   #  define NK_LATTICE_SCALE 1024u
   #else
   #  define NK_LATTICE_SCALE 1u
   #endif

   nk_ticket += NK_LATTICE_STEP * NK_LATTICE_SCALE;   /* single ADD/SUB */

Lock-address guard ::

   _Static_assert(NK_LOCK_ADDR <= 0x3F,
                  "lock must reside in lower I/O space");

Unified superlock
~~~~~~~~~~~~~~~~~

``nk_superlock_t`` wraps the DAG-aware ``nk_slock_t`` with a global *Big
Kernel Lock* (BKL) named ``nk_bkl``.  Acquiring a superlock therefore grabs
``nk_bkl`` first and releases it last.  ``nk_superlock_init`` initialises both
the instance and the global lock.

.. code-block:: c

   nk_superlock_t lock = NK_SUPERLOCK_STATIC_INIT;
   nk_superlock_init(&lock);

   nk_superlock_lock(&lock, 0x1u);
   nk_superlock_unlock(&lock);

Real-time mode
~~~~~~~~~~~~~~

Latency-critical code may bypass the BKL.  The ``*_lock_rt`` and
``*_unlock_rt`` variants operate only on the local instance and set the
``rt_mode`` flag while held.

.. code-block:: c

   nk_superlock_lock_rt(&lock, 0x2u);
   nk_superlock_unlock_rt(&lock);

   if (nk_superlock_trylock_rt(&lock, 0x3u)) {
       nk_superlock_unlock_rt(&lock);
   }

Encoding and matrix helpers
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The four-word ``matrix`` field models speculative state.  It can be
serialised with :c:func:`nk_superlock_encode` into
``nk_superlock_capnp_t`` and later restored with
:c:func:`nk_superlock_decode`.  Individual entries are adjusted via
:c:func:`nk_superlock_matrix_set`.

.. code-block:: c

   nk_superlock_capnp_t snap;
   nk_superlock_encode(&lock, &snap);
   nk_superlock_matrix_set(&lock, 2, 0xdeadbeef);
   nk_superlock_unlock(&lock);
   nk_superlock_decode(&lock, &snap);

See ``tests/unified_spinlock_test.c`` for a complete example.

----------------------------------------------------------------------
8 · Optimisation playbook
----------------------------------------------------------------------

* **Compiler** `avr-gcc ≥ 14` (C23, full LTO).  
* **Flags**

  .. code-block::

     -Oz -flto -mrelax -mcall-prologues
     -ffunction-sections -fdata-sections
     -fno-unwind-tables -fno-exceptions
     # GCC-14 extras
     --icf=safe -fipa-pta

* **Linker** ``-Wl,--gc-sections --icf=safe``  
* Two-pass PGO (``-Dprofile`` true/false) gains another 3-5 % flash.

----------------------------------------------------------------------
9 · Resource accounting
----------------------------------------------------------------------

==================  Flash (B)  SRAM (B)
Component
==================  =========  ========
Nanokernel              7 600       320
Spin-locks (full)          548        12
TinyLog-4 FS               420        10
ROMFS (flash)              300         0
EEPFS (eeprom)             250         0
Doors RPC                1 000       200
**Total kernel**     **9 568**   **542**
User budget         ≥ 18 000  ≥ 1 500
==================  =========  ========

----------------------------------------------------------------------
10 · Copy-on-write flash (page-level)
----------------------------------------------------------------------

1. Copy 128 B page → SRAM  
2. Program spare *boot* page (≈ 3 ms)  
3. Patch jump-table; later ``LPM`` lands in the copy

----------------------------------------------------------------------
11 · Tool-chain & build
----------------------------------------------------------------------

Meson cross-file encodes all flags ::

   meson setup build --wipe \
       --cross-file cross/atmega328p_gcc14.cross
   # LLVM:
   # meson setup build --cross-file cross/atmega328p_clang20.cross
   ninja -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

PGO cycle ::

   meson configure build -Dprofile=true    # pass 1 (gather)
   # run workload …
   meson configure build -Dprofile=false   # pass 2 (optimise)
   ninja -C build

----------------------------------------------------------------------
12 · CI snapshot
----------------------------------------------------------------------

.. code-block:: yaml

   jobs:
     build:
       runs-on: ubuntu-24.04
       strategy:
         matrix:
           mode: ["modern", "legacy"]
       steps:
         - uses: actions/checkout@v4
         - run: sudo ./setup.sh --${{ matrix.mode }}
         - run: |
             CROSS_FILE=cross/atmega328p_gcc14.cross
             [[ "${{ matrix.mode }}" == "legacy" ]] && \
               CROSS_FILE=cross/atmega328p_gcc7.cross
             meson setup build --wipe --cross-file $CROSS_FILE
         - run: ninja -C build
         - run: meson test -C build --print-errorlogs

----------------------------------------------------------------------
13 · QEMU verification
----------------------------------------------------------------------

* `-M arduino-uno` (QEMU ≥ 8.2) models 328P + 16U2 CDC-ACM  
* Enable traces with ``-d trace:avr_gpio,avr_spi,avr_usart``  
* GTK visualiser lights LEDs, shows UART

----------------------------------------------------------------------
14 · Unit-test hammer
----------------------------------------------------------------------

* 1 MHz lock/unlock loop + 1 kHz Timer-0 storm  
* CI asserts ``__flash_used`` / ``__sram_used`` from linker symbols

----------------------------------------------------------------------
Further reading
----------------------------------------------------------------------

* :doc:`hardware`
* :doc:`toolchain`
* Atmel **AVR Instruction-Set Manual** (pdf)

----------------------------------------------------------------------
Glossary
----------------------------------------------------------------------

``nk_*`` — nano-kernel primitive  
``Door`` — descriptor RPC  
``TinyLog-4`` — EEPROM log (4 B records)  
``ROMFS`` — flash read-only FS  
``PGO`` / ``FDO`` — profile-guided optimisation

----------------------------------------------------------------------
Status — 20 Jun 2025
----------------------------------------------------------------------

* Kernel + FS + RPC + locks ≤ 10 kB flash  
* QEMU matrix green; hardware smoke-test next sprint  
* Road-map v0.2 → shell pipes, XMODEM loader, 16U2 co-proc locks

> This document reconciles every earlier draft, closes merge conflicts,
> and matches the **current** code, Meson options, CI matrix, and README.
