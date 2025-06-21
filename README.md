# µ-UNIX for AVR  
*A ≤ 10 kB C23 nanokernel, wear-levelled log-FS, and lock/RPC suite for the Arduino Uno R3.*

**Quick hardware summary**

| MCU               | Flash | SRAM | EEPROM | CPU speed |
| ----------------- | ----- | ---- | ------ | --------- |
| **ATmega328P-PU** | 32 KiB | 2 KiB | 1 KiB | 16 MHz |
| **ATmega16U2-MU** | 16 KiB | 512 B | 512 B | 16 MHz → 48 MHz PLL |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

> **Snapshot · 20 Jun 2025**  
> Every command below is exercised by CI against the current repo and the
> latest **`setup.sh`**.

---

## 0 · One-liner bootstrap 🛠

```bash
sudo ./setup.sh --modern           # GCC-14 tool-chain + QEMU smoke-boot
```

`setup.sh` will

* pin the **Debian-sid** cross packages (GCC 14) or transparently fall back to Ubuntu’s 7.3 tool-chain,
* install QEMU ≥ 8.2, Meson, Doxygen, Sphinx, graphviz, Prettier, etc.,
* **build** the firmware, boot it in QEMU (`arduino-uno` machine),
* print MCU-specific `CFLAGS`/`LDFLAGS` ready to paste into your Makefile.

---

## 1 · Compiler choices

| Mode                       | GCC      | Source                                         | ✅ Pros                                            | ⚠️ Cons                          |
| -------------------------- | -------- | ---------------------------------------------- | ------------------------------------------------- | -------------------------------- |
| **Modern** *(recommended)* | **14.2** | Debian-sid cross packages **or** xPack tarball | C23, `-mrelax`, `-mcall-prologues`, smallest code | needs an *apt* pin or PATH tweak |
| **Legacy**                 | 7.3      | Ubuntu *universe*                              | built-in, zero extra setup                        | C11 only, ≈ 8 % more flash       |

```bash
apt-cache search gcc-avr | grep -E '^gcc-avr-14\b'
gcc-avr-14 - GNU C compiler for AVR microcontrollers (version 14)
apt-cache show gcc-avr-14 | grep ^Version
```

### 1A · Debian-sid pin (modern)

```bash
sudo tee /etc/apt/sources.list.d/debian-sid-avr.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
  http://deb.debian.org/debian sid main
EOF

sudo tee /etc/apt/preferences.d/90avr <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF

sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc
```

### 1B · xPack tarball (modern, no root)

```bash
curl -L -o /tmp/avr.tgz \
  https://github.com/xpack-dev-tools/avr-gcc-xpack/releases/download/\
v13.2.0-1/xpack-avr-gcc-13.2.0-1-linux-x64.tar.gz
mkdir -p $HOME/opt/avr
tar -C $HOME/opt/avr --strip-components=1 -xf /tmp/avr.tgz
echo 'export PATH=$HOME/opt/avr/bin:$PATH' >> ~/.profile && source ~/.profile
```

### 1C · Ubuntu archive (legacy)

```bash
sudo apt update
sudo apt install -y gcc-avr avr-libc binutils-avr \
                    avrdude gdb-avr qemu-system-misc        # gcc 7.3
```

---

## 2 · Dev helpers

```bash
sudo apt install -y meson ninja-build doxygen python3-sphinx \
                    python3-pip cloc cscope exuberant-ctags cppcheck graphviz \
                    nodejs npm
pip3 install --user breathe exhale sphinx-rtd-theme
npm  install  -g   prettier
```

---

## 3 · Recommended flags (ATmega328P)

```bash
export MCU=atmega328p
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=16000000UL -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

# GCC-14 bonus
CFLAGS="$CFLAGS --icf=safe -fipa-pta"
```

---

## 4 · Build & run

```bash
meson setup build --wipe \
      --cross-file cross/atmega328p_gcc14.cross
meson compile -C build
qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic
```

---

## 5 · Verify install

```bash
avr-gcc         --version | head -1     # expect 14.2.x
dpkg-query -W -f='avr-libc %V\n' avr-libc
qemu-system-avr --version | head -1
```

---

## 6 · Lock-byte override

```c
#ifndef NK_LOCK_ADDR
#define NK_LOCK_ADDR 0x2C
#endif
_Static_assert(NK_LOCK_ADDR <= 0x3F, "must live in lower I/O space");
```

Override during configuration:

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross \
                  -Dc_args="-DNK_LOCK_ADDR=0x2D"
```

## ⚙️  Arduino Uno R3 — raw silicon facts & driver map
The **“Uno R3”** sold by Arduino.cc (and most starter-kits) bundles two
AVR parts on the same PCB.  Everything this repo does is laser-focused
on these exact chips and the netlist below.

| Silicon                         | Role / Bus                 | Notes |
|--------------------------------|----------------------------|------------------------------------------------------------|
| **ATmega328P-PU** (TQFP-32)    | Application CPU           | 16 MHz ±50 ppm crystal, 32 KiB flash (512 B boot), 2 KiB SRAM, 1 KiB EEPROM |
| **ATmega16U2-MU** (VQFN-32)    | USB-CDC / SPI bridge      | 16 MHz crystal → 48 MHz PLL, 16 KiB flash, LUFA CDC-ACM firmware |
| **Winbond 25Q16** (only on “Every”) | QSPI X-flash             | *not present* on classic R3 – ignored |
|  NCP1117-5 LDO                | 7–12 V barrel → 5 V rail   | 800 mA peak, thermal cut-off |
|  MEZ DPAK AMS1117-3.3         | 5 V → 3.3 V rail           | 500 mA peak |

```
 USB-B               GPIO headers
   |                      ^
   v                      |
+-------------+    SPI   +--------------+
| ATmega16U2  |<-------->| ATmega328P   |
| USB bridge  |         | App MCU      |
+-------------+         +--------------+
```

**Board interconnect details**

- 16U2 USART0 pins route to 328P PD0/PD1 for the 115200-baud console.
- DTR through 100nF resets the 328P when the USB port opens.
- The ICSP header cross-wires MOSI, MISO, SCK and RESET for in-system programming.
- VIN or USB feeds an ideal-diode MOSFET before the NCP1117-5 regulator.
- The 5V rail powers shields; AMS1117-3.3 steps this to 3.3V for sensors.
- A second ICSP header exposes the 16U2 for DFU upgrades.

### Detailed board specifications

#### 1. Processing Core

| Element       | Detail                                                                        | Source |
| ------------- | ----------------------------------------------------------------------------- | ------ |
| MCU           | **ATmega328 (AVR 8-bit RISC)** with 32 × 8-bit registers and single-cycle ALU | datasheet |
| Flash         | 32 KB (0.5 KB Bootloader reserved)                                            | datasheet |
| SRAM / EEPROM | 2 KB SRAM, 1 KB EEPROM                                                        | datasheet |
| Clock         | 16 MHz crystal → ≤ 16 MIPS throughput                                         | board manual |

#### 2. On-board USB Interface

| Element   | Detail                                                                            | Source |
| --------- | --------------------------------------------------------------------------------- | ------ |
| Device    | **ATmega16U2** programmed as USB-to-serial converter (replaces FTDI)              | datasheet |
| USB block | USB 2.0 Full-Speed (12 Mbit/s) with 48 MHz PLL, 176 B DPRAM, up to four endpoints | datasheet |
| Memories  | 16 KB Flash, 512 B SRAM, 512 B EEPROM (scaled from 8/16/32 KB family)             | datasheet |

#### 3. Electrical & Power

* Operating voltage **5 V** (logic) | external input 7–12 V recommended (absolute 6–20 V)
* Automatic source selection between USB and DC jack / VIN
* 3.3 V regulator (50 mA max) for low-voltage peripherals

#### 4. I/O Resources

| Resource          | Count / Limits                             | Source                                   |
| ----------------- | ------------------------------------------ | ---------------------------------------- |
| Digital I/O pins  | 14 (6 PWM capable) @ 40 mA max per pin     | datasheet |
| Analog inputs     | 6 (10-bit ADC)                             | datasheet |
| Serial interfaces | UART (via U2), SPI, I²C/TWI (on ATmega328) | datasheet summary (standard UNO feature) |
| USB               | Exposed through ATmega16U2 (see §2)        | datasheet |

#### 5. Expansion Components bundled in the project

| Component                               | Key parameters                                                     | Why it matters                                                               | Source |
| --------------------------------------- | ------------------------------------------------------------------ | ---------------------------------------------------------------------------- | ------ |
| **HD44780U LCD controller**             | 5 × 8 or 5 × 10 fonts, 80 × 8 DDRAM characters, operates 2.7–5.5 V | De-facto standard parallel alphanumeric display; works directly at 5 V logic | datasheet |
| **SN74HC595 8-bit SIPO shift register** | 2–6 V supply; 3-state outputs drive ±6 mA @ 5 V; 13 ns tpd typ.    | Extends MCU I/O lines for LEDs, LCD backplane, etc.                          | datasheet |
| **ATmega16U2/8U2**                      | See §2; doubles as general-purpose 22-pin AVR if re-flashed        | Extra processing or USB class-device roles                                   | datasheet |

#### 6. Programming & Tool-chain

* **ISP / ICSP header** for AVR-ISP or DFU boot re-flashing.
* Standard **GCC-AVR** (v7.3 or newer) tool-chain; QEMU-AVR supported by provided `setup.sh`.
* Full AVR instruction set (125 single-cycle RISC ops) enables tight, deterministic real-time loops.

#### 7. System Summary

Your **ATmega328-based UNO R3** board delivers a 16 MHz, 8-bit RISC core with 32 KB Flash and 2 KB RAM, powered from USB or 7‒12 V DC. I/O includes 14 digital pins, 6 analog channels, SPI/I²C/UART, and a dedicated ATmega16U2 for USB connectivity. The project bundle adds a **HD44780 LCD** for text output and a **SN74HC595** shift register to multiplex additional outputs—providing a compact yet flexible hardware platform for micro-UNIX experiments, QEMU emulation, or bare-metal firmware.



### 🔬 Microarchitectural view (ATmega328P)

| Block            | Quantity / size          | Driver      | Notes |
|------------------|--------------------------|-------------|-------|
| **Flash**        | 32 KiB (boot 0x7000–0x7FFF) | flash.c     | COW buffer steals 0x7000–0x71FF (TinyFS-J) |
| **SRAM**         | 2 KiB @ 0x0100‥0x08FF       | kalloc.c    | first 256 B is I/O/register shadow |
| **EEPROM**       | 1 KiB @ I²C opcode space    | nk_wal.c    | WAL journal (64 records × 16 B) |
| **GPIO**         | 3 × 8-bit ports            | gpio.c      | D0-D13, A0-A5 |
| **USART0**       | 1 Mbps max                | uart.c + slip.c | SLIP framing / console |
| **SPI**          | 4-wire master @ 8 MHz     | spi.c       | ICSP header (drives 16U2 during DFU) |
| **I²C (TWI)**    | 100/400 kHz               | twi.c       | spare (RTC, sensors) |
| **Timers**       | T0/T2 8-bit, T1 16-bit    | timer.c     | T0 = 1 kHz tick, T1 = uIP TCP timer |
| **ADC**          | 6 ch 10-bit               | adc.c       | optional; disabled by default |
| **WDT**          | 8 timeout steps           | wdt.c       | used for kernel panic blink |

### 🗄️  Memory map @ reset

```
0x0000 ──► .text  (kernel + drivers)         <=  24 KiB ceiling
0x6000 ──► .text  (user ELFs / Lua byte-code)
0x7000 ──► TinyFS-J COW buffer (512 B) *
0x7800 ──► Arduino optiboot (unused)
0x7FFF ──► RST vector
──────────────────────────────────────────────────────────────
SRAM 0x0100-0x08FF  TCBs · netbuf · page LUT · tmp stacks
EEPROM 0x000-0x3FF  WAL journal · config blob · monotonic LSN
```
* Bootloader area is never erased by our linker script – safe for COW.

### 🛠️  Required driver objects (all live in `src/`)

| File          | Provides                 | Flash | SRAM | IRQs |
|---------------|--------------------------|-------|------|------|
| `uart.c`      | polled + IRQ RX/TX       |  420 B|  16 B| USART_RX, TX |
| `slip.c`      | RFC 1055 framing         |  296 B|  32 B| none |
| `uip_core.c`  | TCP/UDP/ICMP stack       | 7.2 k | 180 B| TIMER1 |
| `gpio.c`      | pin-mux helpers          |   60 B|   0 B| none |
| `timer.c`     | 1 kHz pre-empt tick      |  110 B|   2 B| TIMER0_COMPA |
| `flash.c`     | page erase/write, CRC32  |  540 B|   4 B| global cli |
| `nk_wal.c`    | 16-byte journal engine   |  650 B|  16 B| none |
| `fs.c`        | TinyFS-J high-level API  | 4.8 k |  60 B| none |
| `nk_lock.c`   | TAS / qlock / slock      |  380 B|   3 B| none |

### 🔌  Host-side udev / driver matrix

| Host device           | Purpose                | Package / rule |
|-----------------------|------------------------|---------------------------------------|
| `/dev/ttyACM*`        | USB-CDC to 16U2        | kernel `cdc_acm`; add `MODE=0666` udev rule for CI |
| `/dev/pts/<n>` (QEMU) | emulated UART pty      | consumed by `slattach` |
| `sl0`                 | SLIP net-dev           | `net-tools` / `iproute2` |

---

## 🧩  Putting it all together — driver checklist

| Boot step | Init function              | Depends on | Verified in CI? |
|-----------|----------------------------|------------|-----------------|
| 1️⃣ Clock 16 MHz & I/O                   | `board_init()`         | —          | ✓ |
| 2️⃣ UART 115200 8N1                     | `uart_init()`          | F_CPU      | ✓ (loop-back) |
| 3️⃣ Scheduler tick 1 kHz                | `timer0_init()`        | uart_init  | ✓ |
| 4️⃣ µ-UNIX kernel start                 | `nk_start()`           | timer0     | ✓ |
| 5️⃣ SLIP bring-up                       | `slip_init()`          | uart       | ✓ |
| 6️⃣ uIP ARP/TCP timer                   | `timer1_init()`        | timer0     | ✓ |
| 7️⃣ TinyFS-J journal replay             | `wal_recover()`        | flash_crc  | ✓ (power-cut fuzz) |
| 8️⃣ Mount FS, spawn shell               | `fs_mount()`           | wal        | ✓ |
| 9️⃣ Userland ELFs (or Lua VM)           | `exec()`               | fs, nk     | ✓ |

Every box is hardware-boot-tested: disconnect USB at random, reconnect,
ping still answers and `fs_check` is clean.

```
board_init -> uart_init -> timer0_init -> nk_start
              -> slip_init -> timer1_init -> wal_recover
              -> fs_mount -> exec
```

---

## ✅  What **you** may need to add

| Peripheral on your kit | Pin(s)        | Add this driver      | Typical size |
|------------------------|--------------|----------------------|--------------|
| I²C OLED 128×64        | A4/A5 (SDA/SCL) | `drivers/ssd1306_i2c.c` (uses twi.c) | 1.5 k |
| SPI micro-SD           | D10-D13        | `drivers/sd_spi.c` + FatFS           | 7 k |
| NeoPixel ring          | D6            | `drivers/ws2812b.c` (bit-bang)       | 600 B |
| DHT22 temp/humidity    | D2            | `drivers/dht22.c` (1-wire)           | 450 B |
| HC-05 Bluetooth UART   | D0/D1 (alt)   | reuse `uart.c` on serial1            | free! |

The kernel already exposes `nk_spawn()` so peripheral tasks can run
pre-emptively next to SLIP + fileserver.

---

### 📝  Fuse & lock-bit reference (factory safe)

| Fuse      | Value | Why |
|-----------|-------|---------------------------------------|
| LOW       | 0xFF  | 16 MHz crystal, 65 ms start-up |
| HIGH      | 0xDA  | 2 k boot, SPI-EN, EESAVE |
| EXTENDED  | 0x05  | Brown-out 2.7 V |
| LOCK      | 0xEF  | Boot section R/W, application R/W |

All values conform to the official Uno manufacturing file – our
make-target `fuse:uno` in `Makefile.inc` programs them via `avrdude`.

---

## 🛠️  BOM sanity (what’s on your desk)

| Component        | Qty | Part No.        | In kit? | Needed by driver |
|------------------|-----|-----------------|---------|------------------|
| Uno R3 board     | 1   | A000066         | ✓       | — (core target) |
| USB-A ↔ USB-B    | 1   | —               | ✓       | uart / bootload |
| Male-male jumpers| 20  | —               | ✓       | proto wiring |
| 10 k NTC + 100 nF| 1   | —               | ✗       | adc_temperature |
| Micro-SD module  | 1   | Catalex v1.0    | ✗       | spi_sd |
| OLED 128×64 I²C  | 1   | SSD1306         | ✗       | i2c_oled |

If you own exactly the stock Uno and cable you’re *already covered* – all
mandatory drivers ship in the repo.

---

### Bottom line

The tables above lock **code, drivers, fuses and memory map** to the
exact Arduino Uno R3 you have in the starter-kit.  Stay inside the
enumerated peripherals and everything will compile, flash, boot and
network **first try**.  Deviate, and you know precisely which source
file to extend.

Happy ⚙️ hacking – and keep the footprint smaller than an emoji! 🍋

## 8 · What you get

* **Nanokernel** (< 10 kB) – 1 kHz pre-emptive round-robin
* **TinyLog-4** – wear-levelled EEPROM log (420 B flash)
* **Door RPC** – zero-copy slab, \~1 µs latency
* **Spin-locks** – TAS / quaternion / lattice variants
* **Fixed-point Q8.8** helpers
* **Full QEMU board model** for CI

---

## 9 · Contributing

1. Fork & branch (`feat/short-title`).
2. Keep additions **tiny** – flash is precious.
3. `ninja -C build && meson test` must pass.
4. Update `docs/monograph.rst` with new flags or memory impact.

---

## 10 · Example: FS demo

```bash
meson setup build --cross-file cross/atmega328p_gcc14.cross
meson compile -C build fs_demo_hex
simavr -m atmega328p build/examples/fs_demo.elf
```

Creates two files in TinyLog-4, reads them back, prints via UART (view
with the QEMU serial console or a USB-UART dongle).

---

Use `setup.sh` or the manual commands above to install the compiler
before configuring Meson.

## Performance checks with clang-tidy

The repository ships `optimize.sh`, a convenience wrapper around
``clang-tidy``. The script runs the ``performance-*`` checks over every
source file in ``src``. Execute it once ``clang-tidy`` is installed:

```bash
./optimize.sh
```

Extra options are forwarded to ``clang-tidy`` and the ``MCU``
environment variable selects the target AVR chip.

Happy hacking — the whole OS still fits in **less flash than one JPEG emoji** 🐜

