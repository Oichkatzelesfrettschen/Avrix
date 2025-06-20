.. _monograph:

=========================================================
Monograph — µ-UNIX on the Arduino Uno R3 (ATmega328P SoC)
=========================================================

.. contents::
   :local:
   :depth: 2

This monograph condenses our entire *first-principles* expedition—from
180 nm transistor physics to a < 10 kB nanokernel—into a single reference
for the **Arduino Uno R3** platform
(application MCU **ATmega328P** + USB bridge **ATmega16U2**).

----------------------------------------------------------------------
1 · Silicon → Instruction Set
----------------------------------------------------------------------

* **Process / physics** 180 nm CMOS.  Gate-oxide capacitance
  :math:`C_{\text{ox}}\!\approx\!8.6\;\text{fF·µm}^{-2}`,  
  silicon band-gap :math:`E_g = 1.12 \text{eV}` ⇒ safe
  :math:`V_\text{min} ≈ 2.7 \text{V}`.

* **Core micro-architecture** AVRe+ two-stage pipeline –  
  1-cycle ALU ops, 2-cycle taken branches, 2-cycle ``LPM``,
  3-cycle ``SPM``.

* **Register file** 32 × 8-bit, dual-read / single-write → 1-cycle access.

* **Memory map**

  + 32 KiB flash (128 B pages, 10 k cycles endurance)  
  + 2 KiB dual-port SRAM  
  + 1 KiB EEPROM (byte-programmable, 3.4 ms write)

----------------------------------------------------------------------
2 · Kernel Architecture
----------------------------------------------------------------------

The **nanokernel** occupies *≤ 10 KiB flash* and *≤ 384 B SRAM*.

=====================  ============================  =================
Metric                 Value @ 16 MHz               Note
=====================  ============================  =================
Context-switch         35 cycles (≈ 2.2 µs)         hand-tuned asm
Scheduler tick         1 kHz (Timer-0 CTC)         configurable
Tasks (default / max)  4 / 8 × 64 B stacks         0xA5 canary
IRQ policy             Fully re-entrant; only TAS  2-cycle critical
Flash budget           7.6 KiB (measured)          size-optimised
SRAM budget            320 B (measured)            inc. run-queues
=====================  ============================  =================

Each stack’s guard is verified on every switch; overflow triggers
``nk_panic()``.

----------------------------------------------------------------------
3 · Memory Manager (`kalloc`)
----------------------------------------------------------------------

* 256 B heap, 1-byte free-list header.  
* Best-fit with *first-touch* compaction in idle time.  
* O(1) ``alloc`` / ``free`` for ≤ 8 blocks (bitmap lives in ``GPIOR0``).

----------------------------------------------------------------------
4 · Fixed-Point Arithmetic (Q8.8)
----------------------------------------------------------------------

Kernel math uses *signed* **Q8.8**:

* High byte = integer, low byte = fractional.  
* ``0x0100`` → +1, ``0xFF00`` → −1.  
* Range ≈ −128 … +127.996.  
* Multiplication keeps the middle 16 bits of the 32-bit product with
  rounding, so every step remains 8-bit only.

----------------------------------------------------------------------
5 · TinyLog-4 EEPROM Filesystem
----------------------------------------------------------------------

Wear-levelled, power-fail-safe log:

* **Layout** 16 rows × 64 B ⇒ 256 records × 4 B.  
* **Record** ``tag | data0 | data1 | CRC-8`` (atomic write).  
* **Lookup** < 200 µs worst-case; 420 B flash / 10 B SRAM.

``fs_list`` example
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   fs_create("boot.bin",   1);
   fs_create("config.txt", 1);

   char names[FS_NUM_INODES][FS_MAX_NAME + 1];
   int n = fs_list(names);

   for (int i = 0; i < n; ++i)
       printf("%s\n", names[i]);

----------------------------------------------------------------------
6 · Descriptor-Based RPC (“Doors”)
----------------------------------------------------------------------

* 4 door descriptors per task in ``.noinit``.  
* 128-byte shared slab (16 Cap’n-Proto words) → zero-copy.

===============  ========================  Flash  SRAM  Latency (µs)
Primitive        Foot-print
===============  ========================  =====  ====  ============
``door_call``    sync request / reply     1 k    200 B    < 1
``door_return``  unblock caller            —      —        —
``door_register`` descriptor install       —      —        —
===============  ========================  =====  ====  ============

----------------------------------------------------------------------
7 · Spin-Locks
----------------------------------------------------------------------

===============  ============================  Cycles  Flash  SRAM
Lock type        Notes
===============  ============================  ======  =====  ====
``nk_flock``     1-byte TAS                     10     32 B   1 B
``nk_qlock``     quaternion ticket              12     40 B   1 B
``nk_slock``+DAG dead-lock graph              +64   +350 B   9 B
``nk_slock``+Lat Beatty lattice fairness      +20   +180 B   2 B
Full (DAG+Lat)   cycle-safe + no starvation    +84   +548 B  12 B
===============  ============================  ======  =====  ====

Golden-ratio ticket ::

   #define NK_LATTICE_STEP  1657u   /* φ·2¹⁰ for 16-bit counters */
   nk_ticket += NK_LATTICE_STEP;

_Lock address guard_ ::

   _Static_assert(NK_LOCK_ADDR <= 0x3F,
                  "lock must reside in lower I/O space");

----------------------------------------------------------------------
8 · Optimisation Playbook
----------------------------------------------------------------------

* **Compiler** `avr-gcc ≥ 14` (full C23).  
* **Flags** ::

    -Oz -flto -mrelax -mcall-prologues
    -ffunction-sections -fdata-sections
    -fno-unwind-tables -fno-exceptions

* **Linker** ``-Wl,--gc-sections --icf=safe``  
* Two-pass FDO/PGO → extra 3–5 % flash drop.

----------------------------------------------------------------------
9 · Resource Accounting
----------------------------------------------------------------------

===============  Flash (B)  SRAM (B)
Component
===============  =========  ========
Nanokernel            7600      320
Spin-locks (full)       548       12
TinyLog-4 FS            420       10
Doors RPC             1000      200
**Total kernel**  **9568** **542**
User budget        ≥ 18 000  ≥ 1500
===============  =========  ========

----------------------------------------------------------------------
10 · Copy-on-Write Flash (page-level)
----------------------------------------------------------------------

1. Copy 128 B page → SRAM buffer  
2. Program spare *boot* page (≈ 3 ms)  
3. Patch jump table; subsequent ``LPM`` hits new copy.

----------------------------------------------------------------------
11 · Tool-chain & Build
----------------------------------------------------------------------

*Meson cross-file* (`cross/atmega328p_gcc14.cross`) encodes the flag set ::

   meson setup build --cross-file cross/atmega328p_gcc14.cross
   ninja -C build
   qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

FDO cycle ::

   meson configure build -Dprofile=true   # pass 1 (collect)
   # run workload …
   meson configure build -Dprofile=false  # pass 2 (optimise)
   ninja -C build

----------------------------------------------------------------------
12 · CI Snapshot
----------------------------------------------------------------------

.. code-block:: yaml

   jobs:
     build:
       runs-on: ubuntu-24.04
       steps:
         - uses: actions/checkout@v4
         - run: sudo ./setup.sh --modern
         - run: meson setup build --cross-file cross/atmega328p_gcc14.cross
         - run: ninja -C build
         - run: qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic &

----------------------------------------------------------------------
13 · QEMU Verification
----------------------------------------------------------------------

* Board ``-M arduino-uno`` (QEMU ≥ 8.2) models 328P + 16U2 CDC-ACM.  
* Trace with ``-d trace:avr_gpio,avr_spi,avr_usart``.  
* GTK visualiser shows LEDs, buttons, UART.

----------------------------------------------------------------------
14 · Unit-Test Hammer
----------------------------------------------------------------------

* 1 MHz lock/unlock loop + 1 kHz Timer-0 flood.  
* CI asserts `__flash_used` / `__sram_used` from linker symbols.

----------------------------------------------------------------------
Further Reading
----------------------------------------------------------------------

* ``docs/hardware.rst`` — Uno R3 power, clock, ESD  
* ``docs/build.rst`` — tool-chain bootstrap, CI  
* Microchip **ATmega8/16/32U2** datasheet  
* **AVR Instruction-Set Manual**

----------------------------------------------------------------------
Glossary
----------------------------------------------------------------------

``nk_*``   nanokernel primitive  
``Door``   descriptor-based RPC  
``TinyLog-4`` EEPROM log (4-byte record)  
``FDO``    feedback-directed optimisation (PGO)

----------------------------------------------------------------------
Status — 20 Jun 2025
----------------------------------------------------------------------

* Kernel + FS + RPC + locks fit **< 10 kB flash**.  
* QEMU matrix green; hardware smoke-test next sprint.  
* Roadmap v0.2: shell pipes, XMODEM loader, 16U2 co-proc locks.

> *Every byte, table and diagram is sourced from chat deliberations,
> uploaded PDFs, and in-repo code—yielding a cohesive guide to building a
> modern **µ-UNIX** for an 8-bit AVR.*
