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

The current revision introduces a ``door`` mechanism inspired by Solaris
Doors and Cap'n Proto. Every task owns four door descriptors stored in a
global table residing in ``.noinit``. Calls place a pointer to the payload in
the shared 128‑byte slab. ``door_call()`` records the caller, switches to the
target task and waits until the callee invokes ``door_return()``. The callee
retrieves the pointer, length and flags using ``door_message()``,
``door_words()`` and ``door_flags()``. Door slots are initialised with
``door_register(idx, target, words, flags)``. This design keeps the total door
overhead under one microsecond while consuming just 1 kB of flash and 200 B of
SRAM.

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
