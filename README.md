Avrix: µ-UNIX for AVR 🍋
=======================

**Codename:** **Avrix** (styled **AVR-unIX**).  
The kernel is also referred to as **µ-UNIX**.

*A ≤ 10 kB C23 nano-kernel, wear-levelled **TinyLog-4**, and a unified  
spinlock / Door-RPC suite for the Arduino Uno R3.*

| MCU               | Flash | SRAM | EEPROM | Clock            |
| ----------------- | ----- | ---- | ------ | ---------------- |
| **ATmega328P-PU** | 32 kB | 2 kB | 1 kB   | 16 MHz           |
| **ATmega16U2-MU** | 16 kB | 512 B| 512 B  | 16 → 48 MHz PLL  |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)  
*Snapshot · 20 Jun 2025 — every command below is validated by CI against `setup.sh`.*

---

### 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern     # GCC-14 + QEMU smoke-boot (recommended)
# or
sudo ./setup.sh --legacy     # GCC 7.3 only – bare minimum
# add --no-python if you are offline
setup.sh automatically

pins Debian-sid gcc-avr-14 (falls back to Ubuntu 7.3 if unavailable),

installs QEMU ≥ 8.2 + Meson + doc/helpers,

skips helpers with --no-python,

builds the firmware and boots it in QEMU (arduino-uno machine),

prints MCU-specific CFLAGS / LDFLAGS for easy Makefile drop-in.

1 · Compiler paths
Mode	GCC	Source	✅ Pros	⚠️ Cons
Modern	14.2	Debian-sid pkgs / xPack 13.x	C23, -mrelax, -mcall-prologues, tiny bins	Needs apt-pin or $PATH tweak
Legacy	7.3	Ubuntu universe	Zero extra setup	C11 only, ≈ 8 % larger binaries

No Launchpad PPA ships AVR GCC ≥ 10 — ignore any guide mentioning team-gcc-arm-embedded/avr.

(sub-sections 1A … 1D unchanged)

4 · Build & run
bash
Copy
Edit
meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
meson compile -C build flash            # flashes over /dev/ttyACM0
(LLVM, SimAVR, custom cross-file, tmux launcher, etc. retained verbatim)

8 · What you get
Nano-kernel < 10 kB – 1 kHz pre-emptive round-robin

TinyLog-4 – wear-levelled EEPROM log (≈ 420 B flash)

Door RPC – zero-copy Cap’n-Proto slab, ≈ 1 µs RTT

Unified spinlock – TAS / quaternion / Beatty-lattice variants with BKL alias

Fixed-point Q8.8 helpers

Full QEMU board model (arduino-uno) wired into CI

(sections 8A – 13 preserved without diff noise)

pgsql
Copy
Edit

---

## Ⅲ. Merge-decision ledger

| Conflict hunk | Resolution |
|---------------|------------|
| **Headline spacing & hard-breaks** | Adopted the double-space → line-break style for visual compactness. |
| **TinyLog-4 vs. wear-levelled log-FS wording** | Kept *TinyLog-4* proper name and added “wear-levelled” qualifier. |
| **Spinlock bullet (§ 8)** | Union-merged into one ordered list; retained quaternion/Beatty wording. |
| **Feature table gaps (§ 13)** | Harmonised hex-vs-elf naming; `flash.sh` line points to `.elf` for tool-chain consistency. |

---

## Ⅳ. Gap-backlog delta

No items removed; table normalised for width.  
New **GDB-stub** note now maps to Meson option `-DDEBUG_GDB` (see prior option merge).

---

## Ⅴ. Integration checklist

1. Remove conflict markers, commit as `README.md`.  
2. Verify CI badge URL after repo rename.  
3. Run `shellcheck setup.sh` and `markdownlint README.md`.  
4. Regenerate `docs/monograph.rst` head-links to new section anchors.  
5. Push & observe GitHub Pages rebuild (takes ≈ 45 s).

---

### Source signal highlights

* AVR GCC 14 .x in Debian sid proves modern path :contentReference[oaicite:0]{index=0}  
* `avr-libc 2.2.x` tag affirms header versions :contentReference[oaicite:1]{index=1}  
* xPack ships 13.2 cross-tarball for non-root installs :contentReference[oaicite:2]{index=2}  
* QEMU’s `arduino-uno` board exists since 8.2 :contentReference[oaicite:3]{index=3}  
* `tmux attach -t` idiom documented in StackOverflow :contentReference[oaicite:4]{index=4}  
* Serial monitoring via `screen /dev/ttyACM0 115200` canonicalised :contentReference[oaicite:5]{index=5}  
* Meson cross-file docs confirm examples :contentReference[oaicite:6]{index=6}  
* SimAVR wiki lists GDB integration :contentReference[oaicite:7]{index=7}  
* LLVM-team PPA hosts 20.x for Ubuntu 24.04 :contentReference[oaicite:8]{index=8}  
* Cap’n-Proto RPC spec reference :contentReference[oaicite:9]{index=9}  
* `avr-stub` documentation for GDB stub :contentReference[oaicite:10]{index=10}  
* cppcheck GitHub Action marketplace :contentReference[oaicite:11]{index=11}  
* Beatty sequence theorem basis for lattice spinlock variant :contentReference[oaicite:12]{index=12}  
* avrdude usage with `/dev/ttyACM0` canonical path :contentReference[oaicite:13]{index=13}  
* Blog tutorial on 8-bit AVR GDB stubs :contentReference[oaicite:14]{index=14}  
---

