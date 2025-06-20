Monograph: µ-UNIX on ATmega328P
===============================

.. _monograph:

This page summarises the complete first-principles design of the µ-UNIX
platform targeting the Arduino Uno R3.  It distils the theory discussed in
the repository's history into a coherent description.  The full text is
available in the project archive.

Silicon to Instruction Set
--------------------------

* 180 nm CMOS process, 1--2 fF/µm² gate capacitance.
* Two-stage pipeline with single-cycle ALU ops and two-cycle branches.
* Harvard memory: 32 KB flash, 2 KB SRAM, 1 KB EEPROM.

Kernel Architecture
-------------------

The nanokernel reserves 10 KB of flash and under 384 B of SRAM.  Context
switches take 35 cycles.  A round-robin scheduler drives up to eight tasks
with 64-byte stacks.  Filesystem structures mirror Unix V7 but reside in
RAM for simplicity.

Optimisation Techniques
-----------------------

* GCC 14 with ``-Oz -flto -mrelax`` for maximum code density.
* ``-mcall-prologues`` shares common ISR prologs.
* Software pipelining hides ``LPM`` latency.

Resource Accounting
-------------------

Total flash usage is under 28 KB leaving space for user programs.  SRAM is
statically partitioned: 640 B kernel, 640 B shell, 512 B stacks, 256 B heap.

Descriptor-Based RPC
--------------------

The current revision introduces a "door" mechanism inspired by Solaris
Doors and Cap'n Proto. Each task owns four door descriptors mapped to a
128‑byte shared message slab. A single ``door_call()`` copies only the
pointer to the payload, switches to the callee and returns in about fourteen
cycles. The slab lives in ``.noinit`` so soft resets preserve in‑flight
messages. This zero-copy design keeps latency below one microsecond while
consuming just 1 kB of flash and 200 B of SRAM.

Copy-on-Write Flash
-------------------

Forked tasks share flash pages until a write occurs. On the first write the
nanokernel allocates a temporary RAM buffer, modifies the page and reprograms
it into a spare boot section location. Subsequent reads remain fast while
writes incur at most the 3 ms page programming delay.

Further Reading
---------------

See the README for toolchain setup and the :doc:`hardware` page for a deeper
walkthrough of the Uno R3 board.
