
Avrix — µ-UNIX for AVR 🍋
========================

**Codename:** **Avrix** (styled **AVR-unIX**).  
The kernel itself is called **µ-UNIX**.

*A Scalable Embedded POSIX System (8-bit to 32-bit).*

| Profile | MCU Class | RAM | Key Features |
| :--- | :--- | :--- | :--- |
| **Low (PSE51)** | 8-bit (ATmega328) | < 2 KB | Single-Task, No FS, No Net |
| **Mid (PSE52)** | 8/16-bit (ATmega1284) | > 16 KB | Multi-Thread, VFS, Minimal Net |
| **High (PSE54)** | 32-bit (ARM Cortex-A) | > 256 KB | Full POSIX, MPU, TCP/IP |

[![CI](https://github.com/your-org/avrix/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/avrix/actions)

## 0 · Quick Start (Repo Modulator) 🛠

Select a profile to configure the OS for your target hardware:

```bash
# 1. Low Profile (8-bit, Minimal)
meson setup build_low --cross-file config/profiles/low.ini
meson compile -C build_low

# 2. Mid Profile (Multi-threaded, VFS)
meson setup build_mid --cross-file config/profiles/mid.ini
meson compile -C build_mid

# 3. High Profile (Full POSIX)
meson setup build_high --cross-file config/profiles/high.ini
meson compile -C build_high
```

## 1 · Configuration System

Avrix uses a "Repo Modulator" architecture to scale. Configuration is driven by `meson_options.txt` and profile files.

**Key Options:**
- `kernel_sched_type`: `single` (Low), `preempt` (Mid/High)
- `fs_enabled`: Enable Virtual Filesystem
- `net_enabled`: Enable Networking Stack
- `ipc_door_enabled`: Enable Door RPC

See `config/profiles/` for default configurations.

## 2 · One-liner bootstrap

```bash
sudo ./setup.sh --modern      # GCC-14 + QEMU smoke-boot
```

`setup.sh` pins Debian-sid `gcc-avr-14` and installs QEMU.

---

## 3 · Architecture & Modules

### 3.1 · Portable Kernel (Phase 4)
- `kernel/sched/`: Configurable scheduler (Single-task vs Preemptive)
- `kernel/sync/`: Spinlocks & Mutexes (Stubbed in Low profile)
- `kernel/mm/`: Memory Management (kalloc)

### 3.2 · Drivers (Phase 5)
- `drivers/fs/`: VFS layer supporting ROMFS and EEPFS
- `drivers/net/`: IPv4/SLIP stack (RFC 1071 checksums)
- `drivers/tty/`: Ring-buffer UART driver

### 3.3 · Status
- ✅ **Low Profile**: Verified (< 2KB RAM)
- ✅ **Mid Profile**: Verified (Multi-threaded)
- ✅ **High Profile**: Verified (Full POSIX API stubs)
- 🚧 **Shell**: Planned (Phase 2)
- 🚧 **MPU**: Planned (Phase 2)

See **[ROADMAP.md](ROADMAP.md)** for future plans.

---

## 4 · Documentation

- **[ARDUINO_CHIPSETS.md](ARDUINO_CHIPSETS.md)**: Hardware targets
- **[PORTING_GUIDE.md](PORTING_GUIDE.md)**: Architecture porting
- **[BUILD_GUIDE.md](BUILD_GUIDE.md)**: Build system details
- **[ROADMAP.md](ROADMAP.md)**: Future architecture plans

---

## 5 · License

MIT. See `LICENSE` for details.
