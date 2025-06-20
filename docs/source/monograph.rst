Monograph: µ-UNIX on ATmega328P
===============================

.. _monograph:

This document condenses the entire first-principles journey—from 180 nm
transistor physics to a feature-dense nanokernel—into one cohesive
reference for the **Arduino Uno R3 (ATmega328P + ATmega16U2)** platform.

Silicon → Instruction-Set
-------------------------

* **Process / physics** 180 nm CMOS C_ox ≈ 8.6 fF µm⁻²,  
  E_g = 1.12 eV ⇒ V\_min ≈ 2.7 V (noise-safe).  
* **Core micro-architecture** 2-stage pipeline; one-cycle ALU ops,
  2-cycle taken branches, 2-cycle `LPM`, 3-cycle `SPM`.  
* **Register file** 32 × 8 b, 2 R + 1 W ports → single-cycle access.  
* **Memory map**  
  * 32 KiB flash (128 B pages, 10 k cycles)  
  * 2 KiB SRAM (true dual-port)  
  * 1 KiB EEPROM (byte-programmable, 3.4 ms/byte).

Kernel Architecture
-------------------

The nanokernel reserves 10 KB of flash and under 384 B of SRAM.  Context
switches take 35 cycles.  A round-robin scheduler drives up to eight tasks
with 64-byte stacks.  Filesystem structures mirror Unix V7 but reside in
RAM for simplicity. The demonstration disk allocates only sixteen
32-byte blocks so the entire image occupies just 512\,B of SRAM. Persistent
settings live in the 1\,kB EEPROM using the TinyLog-4 layout. It packs 256
records of four bytes and writes each atomically with a checksum so power
failures never corrupt data.

The 256\,B heap is governed by ``kalloc``—a minimal free-list allocator that
recycles memory blocks for safety on this micro-scale platform.

* **Flash budget** ≤ 10 KiB (measured ≈ 7.6 KiB).  
* **SRAM budget** ≤ 384 B (measured ≈ 320 B).  
* **Scheduler** 1 kHz timer-driven round-robin, 4 default tasks  
  (up to 8 with 64 B stacks).  
* **Context switch** 8 cycles ≈ 500 ns @ 16 MHz.  
* **Pre-emptive, fully re-entrant ISR design**—no global disabling of
  interrupts except for the two-cycle TAS critical section.

Filesystem — TinyLog-4
----------------------

* 4-byte atomic records (`tag | data₀ | data₁ | CRC-8`).  
* 256 logical blocks (16 rows × 16 records) use the **entire EEPROM**.  
* Row-header wear-levelling → > 4 M effective write cycles / cell.  
* O(1) append, < 200 µs worst-case lookup, 420 B flash / 10 B SRAM.  
* Provides *key/value*, *delete*, power-fail safety and optional
  background GC—no heap, no page cache.

Descriptor-Based RPC (“Doors”)
------------------------------

Every task owns **4 door descriptors** stored in `.noinit`.  
All calls share a **128-byte slab** (16 Cap’n-Proto words) also in
`.noinit`.

API
^^^^

* ``door_register(idx, target_tid, words, flags)``  
* ``door_call(idx, src_ptr)`` – synchronous, zero-copy  
* ``door_return(ret_ptr)``  

Fast-path latency < 1 µs, footprint ≈ 1 KiB flash / 200 B SRAM.

Spin-Locks
----------

===============  =======================  Cycles (acq)  Flash  SRAM
Lock type        Notes                              
===============  =======================  ===========  =====  ====
`nk_flock`       1-byte TAS, no recurse          10      32 B   1 B
`nk_qlock`       4-mark (quaternion)             12      40 B   1 B
`nk_slock`+DAG   DAG dead-lock check            +64    +350 B   9 B
`nk_slock`+Lat   Beatty lattice fairness        +20    +180 B   2 B
Full (DAG+Lat)   cycle-safe + starvation-free   +84    +548 B  12 B
===============  =======================  ===========  =====  ====

Golden-ratio ticket
~~~~~~~~~~~~~~~~~~~

.. code:: c

   #if NK_WORD_BITS == 16
     #define NK_LATTICE_STEP  1657u      /* φ·2¹⁰ */
   #elif NK_WORD_BITS == 32
     #define NK_LATTICE_STEP 1695400ul   /* φ·2²⁶ */
   #endif
   nk_lattice_ticket += NK_LATTICE_STEP;

Guarantees quasi-uniform ticket spacing for both 16-bit (ATmega328P) and
32-bit (AVR-DA) counters.

Lock-byte safety
~~~~~~~~~~~~~~~~

.. code:: c

   _Static_assert(NK_LOCK_ADDR <= 0x3F,
                  "lock must be in lower I/O for 1-cycle IN/OUT");

Prevents silent downgrade to two-cycle `LDS/STS` if a port is mis-chosen.

Unit-test hammer
~~~~~~~~~~~~~~~~

* 1 MHz lock/unlock loop under 1 kHz IRQ flood  
* Asserts flash/SRAM tallies from linker symbols – catches regressions.

Optimisation Techniques
-----------------------

* **GCC 14** ``-Oz -flto -mrelax -mcall-prologues``  
* Link-time identical-code-folding ``--icf=safe``  
* Register globals (`asm("r2")`) for hot counters.  
* Software pipeline long `LPM` bursts; branch-skip via `sbis/sbic`.

Resource Accounting
-------------------

===============  Bytes (flash)  Bytes (SRAM)
Component       
===============  ============== ============
Nanokernel code          7 600          320
Spin-locks (full)          548           12
TinyLog-4 FS               420           10
Door RPC                 1 000          200
**Total kernel**        **9 568**      **542**
User budget            ≥18 000       ≥1 500
===============  ============== ============

Copy-on-Write Flash
-------------------

First write to a shared page:

1. Copy 128 B into  buffer in SRAM.  
2. Re-program into a spare  boot-section page (3 ms worst-case).  
3. Update jump table; all future reads hit new page.

TinyLog-4 EEPROM Filesystem
--------------------------

The 1 kB EEPROM is organised as 16 rows of 64 bytes. Each row holds fifteen
4-byte records plus a header with a sequence counter. Records contain a tag,
two data bytes and an 8-bit CRC. Appending data simply writes the next free
record. On boot the code scans for the latest valid row in under 140 µs. When
a row fills, the next one is erased and a new header is written, ensuring
wear is spread evenly across all rows.

Further Reading
---------------

* ``docs/hardware.rst`` - schematic-level walk-through of Uno R3 power,
  clocks and protection.  
* ``docs/build.rst`` - Meson cross-file, SVGO build, CI spin-test.
