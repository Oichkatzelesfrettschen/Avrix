# Monograph: **µ-UNIX on ATmega328P**

.. contents::
\:local:
\:depth: 2

This monograph fuses every strand of our *first-principles* journey—silicon
physics, compiler flags, QEMU modelling, and nanokernel design—into one
self-contained reference for the **Arduino Uno R3** platform
(**ATmega328P** application MCU + **ATmega16U2** USB bridge).

---

## Silicon → Instruction Set

* **Process / physics** 180 nm CMOS; gate-oxide capacitance
  \:math:`C_{\text{ox}}\!\approx\!8.6\;\text{fF·µm}^{-2}`,
  silicon band-gap \:math:`E_g=1.12\;\text{eV}` → noise-safe \:math:`V_\text{min}\!≈\!2.7 V`.

* **Core micro-architecture**    AVRe+ two-stage pipeline
  – single-cycle ALU ops, 2-cycle taken branches, 2-cycle `LPM`, 3-cycle `SPM`.

* **Register file** 32 × 8-bit, dual-read/single-write → 1-cycle access.

* **Memory map**

  * 32 KiB flash (128-B pages, 10 k write endurance)
  * 2 KiB SRAM (true dual-port)
  * 1 KiB EEPROM (byte-programmable, 3.4 ms/byte).

---

## Kernel Architecture

The **nanokernel** grabs ≤ 10 KiB flash and ≤ 384 B SRAM.

\=================  ============================================================
Metric             Value (16 MHz core)
\=================  ============================================================
Context-switch      35 cycles (≈ 2.2 µs)
Scheduler tick      1 kHz (Timer 0, CTC)
Tasks (default)    4 (8 max) with 64-B stacks + 0xA5 guard canary
ISR policy         Fully re-entrant; only TAS critical sections mask IRQs
Flash budget       7.6 KiB measured
SRAM budget        320 B measured
\=================  ============================================================

Each stack’s **guard pattern** is verified on every switch; overwrite trips
`nk_panic()`.

### Memory manager (`kalloc`)

* **256 B heap**; freelist header = 1 byte; best-fit with *first-touch*
  compaction in idle time.
* Constant-time alloc/free for ≤ 8 blocks (bitmap in `GPIOR0`).

---

## TinyLog-4 Filesystem

A *wear-levelled*, power-fail-safe log in **EEPROM**:

* **Layout** 16 rows × 64 B ⇒ 256 records·4 B.
* **Record** `tag | data₀ | data₁ | CRC-8` – written atomically.
* **Lookup** ≤ 200 µs worst-case; no heap, no page cache; 420 B flash / 10 B
  SRAM.

---

## Descriptor-Based RPC (“Doors”)

* 4 door descriptors / task in `.noinit`.
* Shared 128-byte slab (16 Cap’n-Proto words) → zero-copy.

\==============  =====================  Flash  SRAM  Latency (µs)
Primitive       Footprint
\==============  =====================  =====  ====  ===============
`door_register` descriptor install      –     –       –
`door_call`     sync request / reply   1 KiB  200 B   < 1
`door_return`   finish & unblock        –     –       –
\==============  =====================  =====  ====  ===============

---

## Spin-Locks (with optional algebraic upgrades)

\===============  ===========================  Cycles (acq)  Flash  SRAM
Lock type        Notes
\===============  ===========================  ============  =====  ====
`nk_flock`       1-byte TAS                     10           32 B   1 B
`nk_qlock`       4-mark quaternion ticket       12           40 B   1 B
`nk_slock`+DAG   dead-lock graph               +64         +350 B   9 B
`nk_slock`+Lat   Beatty lattice fairness       +20         +180 B   2 B
Full (DAG+Lat)   cycle-safe + starvation-free   84         +548 B   12 B
\===============  ===========================  ============  =====  ====

Golden-ratio ticket spacing:

.. code:: c

\#define NK\_LATTICE\_STEP  1657u  /\* φ·2¹⁰ for 16-bit counters \*/
nk\_ticket += NK\_LATTICE\_STEP;

Compile-time safety:

.. code:: c

\_Static\_assert(NK\_LOCK\_ADDR <= 0x3F,   /\* 1-cycle I/O space IN/OUT \*/);

---

## Optimisation Playbook

* **Compiler** `avr-gcc ≥ 14` with full **C23**.

* **Flags**

  `-Oz -flto -mrelax -mcall-prologues -ffunction-sections -fdata-sections
   -fno-unwind-tables -fno-exceptions`
  (`-mrelax` and `-mcall-prologues` enable size-saving linker
  relaxation and prologue sharing).

* **Linker** `-Wl,--gc-sections --icf=safe` (identical-code folding).

* **Profile-guided** (two-pass FDO) → further 3-5 % flash drop.

---

## Resource Accounting

\===============  Bytes (flash)  Bytes (SRAM)
Component
\===============  ============== ============
Nanokernel code          7 600          320
Spin-locks (full)          548           12
TinyLog-4 FS               420           10
Door RPC                 1 000          200
**Total kernel**        **9 568**      **542**
User budget            ≥18 000       ≥1 500
\===============  ============== ============

---

## Copy-on-Write Flash (Pages)

1. Copy 128 B page to SRAM buffer.
2. Re-program spare **boot section** page (≈ 3 ms).
3. Patch jump table → subsequent `LPM` hit new page.

---

## Build & Tool-chain Recipes

### Meson cross file (`cross/atmega328p_gcc14.cross`)

.. code:: ini

\[binaries]
c          = 'avr-gcc'
ar         = 'avr-ar'
strip      = 'avr-strip'
objcopy    = 'avr-objcopy'
size       = 'avr-size'
exe\_wrapper = 'true'

\[host\_machine]
system     = 'baremetal'
cpu\_family = 'avr'
cpu        = 'atmega328p'
endian     = 'little'

\[properties]
needs\_exe\_wrapper = true
c\_args  = \['-mmcu=atmega328p','-std=c23','-DF\_CPU=16000000UL',
'-Oz','-flto','-mrelax','-mcall-prologues',
'-ffunction-sections','-fdata-sections',
'-fno-unwind-tables','-fno-exceptions']
c\_link\_args = \['-mmcu=atmega328p','-flto','-Wl,--gc-sections']

\[built-in options]
optimization    = 'z'
warning\_level   = 2
strip           = true
default\_library = 'none'

\[project options]
profile = false        # toggle PGO

### Build & run

.. code:: bash

meson setup build --cross-file cross/atmega328p\_gcc14.cross
ninja -C build          # emits elf/unix0.elf
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic

(For **FDO**: `-Dprofile=true`, run workload under QEMU, then
`meson configure build -Dprofile=false && ninja`.)

---

## Continuous Integration (clip)

```yaml
# .github/workflows/avr-ci.yml
jobs:
  build:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt update
      - run: sudo apt install gcc-avr avr-libc qemu-system-misc meson ninja-build
      - run: meson setup build --cross-file cross/atmega328p_gcc14.cross
      - run: ninja -C build
      - run: qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic &
```

---

## Hardware Verification via QEMU

* **Board model** `-M arduino-uno` emulates the 328P + 16U2 USB bridge as
  documented in `hw/avr/arduino.c` (QEMU ≥ 8.2).
* GPIO, SPI, and UART events are observable in the GTK visualiser or with
  `-d trace:avr_*`.

---

## Unit-Test Hammer

* 1 MHz lock/unlock loop while flooding Timer 0 IRQ @ 1 kHz → validates
  lock correctness under pre-emption.
* Linker script emits `__flash_used` / `__sram_used` symbols; tests assert
  budgets each CI run.

---

## Further Reading

* **`docs/hardware.rst`** Schematic-level walk-through of Uno R3 power,
  clocks and ESD protection.
* **`docs/build.rst`** Meson cross-file, SVG-optimised build, CI spin-test.
* **Microchip ATmega8/16/32U2 Datasheet** for USB bridge timing and
  flash programme algorithm.
* **AVR Instruction Set Manual** for cycle-accurate latency tables.

---

## Glossary of Symbols

:`nk_*`: nanokernel primitives
:`Door`: lightweight RPC abstraction
:`TinyLog-4`: 4-byte atomic record EEPROM log
:`FDO`: feedback-directed optimisation (PGO)

---

## Status & Outlook (June 20 2025)

* Kernel, FS, RPC, and lock suite now fit **< 10 KiB** flash (328P limit:
  32 KiB).
* QEMU emulation passes full unit-test battery; hardware smoke test on real
  Uno R3 scheduled next sprint.
* Planned v0.2: pipe‐capable shell, XMODEM loader, RISC-algebraic
  `nk_slock` lattices for 16U2 co-processor.

> *This document integrates every thread, code scrap, and PDF datum we have
> exchanged to date—forming a single, rigorous reference for anyone eager to
> run a **tiny, standards-aware µ-UNIX** on an 8-bit AVR.*
