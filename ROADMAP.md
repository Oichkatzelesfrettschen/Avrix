# Avrix Roadmap & Architecture Status

This document outlines the roadmap for evolving Avrix into a fully scalable Embedded POSIX system, ranging from 8-bit microcontrollers to 32-bit application processors.

## ✅ Phase 1: Repo Modulator & Profiles (Completed)

The foundation for scalability has been established via the **Repo Modulator** configuration system.

*   **Configuration System**: Build-time configuration via `meson_options.txt` and `avrix-config.h`.
*   **Profiles**:
    *   **Low (PSE51)**: 8-bit, <2KB RAM. Single-task, no overhead.
    *   **Mid (PSE52)**: 16/32-bit, >16KB RAM. Multi-threaded, VFS, Networking.
    *   **High (PSE54)**: 32-bit, >256KB RAM. Full POSIX, simulated processes.
*   **Harmonization**: Unified headers with inline stubs for disabled features (no `#ifdef` hell in user code).
*   **Modularity**: Kernel and Drivers refactored to respect feature toggles.

## 🚧 Phase 2: High-Tier Enhancements (Todo)

To fully realize the "High-End" vision, the following subsystems need expansion:

### 1. Shell (CLI)
*   **Goal**: Provide a UNIX-like maintenance interface.
*   **Requirement**: Minimal `sh` implementation accessible via TTY.
*   **Features**: `ls`, `cat`, `echo`, `ps`, `mount`.
*   **Target**: Mid (optional) & High profiles.

### 2. Process Abstraction & Isolation
*   **Goal**: True process separation beyond simple threading.
*   **Current State**: Global File Descriptor table (`vfs.c`), shared memory space.
*   **Requirements**:
    *   **Per-Process FD Table**: Move `fds[]` from global `vfs_state` to `process_t` struct.
    *   **MPU Integration**: Use `hal_mpu_*` to protect kernel memory and isolate task stacks.
    *   **Process Loader**: (Optional) Ability to load code segments.

### 3. Advanced Networking
*   **Goal**: Full TCP/IP stack for IoT applications.
*   **Current State**: Minimal IPv4/SLIP implementation.
*   **Requirement**: Integrate **lwIP** or **uIP** as a "Package".
*   **Features**: TCP, UDP, DNS, DHCP, Sockets API (`socket()`, `bind()`, `connect()`).

### 4. Dynamic Loading
*   **Goal**: Runtime extensibility.
*   **Requirement**: Simple ELF loader or bytecode interpreter.
*   **Target**: High-end chips with external RAM (SDRAM/PSRAM).

## 📋 Feature Matrix

| Feature | Low (8-bit) | Mid (16/32-bit) | High (32-bit) | Status |
| :--- | :---: | :---: | :---: | :--- |
| **Scheduler** | Single-Task | Preemptive | Preemptive/SMP | ✅ Done |
| **Filesystem** | None/Stub | ROMFS/EEPFS | VFS + FAT/LittleFS | ✅ VFS/ROM/EEP Done |
| **Network** | None | Minimal IP | TCP/IP Stack | 🚧 Minimal Done |
| **Shell** | None | Optional | Yes | ❌ Pending |
| **Protection** | None | Stack Canaries | MPU/Privilege | 🚧 Partial |
| **Linking** | Static | Static | Static/Dynamic | ❌ Static Only |

## 🛠 Implementation Plan for Contributors

1.  **Shell**: Start by creating `packages/shell` module using `tty` driver.
2.  **MPU**: Implement `kernel/memguard.c` using HAL primitives.
3.  **Networking**: Port lwIP `netif` driver to use `drivers/tty`.
4.  **Process**: Refactor `scheduler.c` to group threads into processes.
