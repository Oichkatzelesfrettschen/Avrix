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

Further Reading
---------------

See the README for toolchain setup and the :doc:`hardware` page for a deeper
walkthrough of the Uno R3 board.
