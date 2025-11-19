# ─── src/meson.build ──────────────────────────────────────────────────
#
# Cross-vs-native strategy
# ------------------------
# • **AVR / cross build** → `libavrix.a`   (full kernel + ASM)
#   This archive is the firmware that ends up on the Uno.
#
# • **Native build**      → `libavrix_host.a`
#   Hosts docs / unit-tests; contains only portable helpers so we do
#   not pull <avr/io.h>.  AVR-specific sources are still *compiled* with
#   `-fsyntax-only` to detect regressions, but never archived.
#
# • `libfs.a` (RAM FS only) is *always* produced for the host so tests
#   can link even when the parent build is a cross compile.
#
# NOTE: `inc` comes from the top-level meson.build
#       (`inc = include_directories('include')`).

# ───────────────────────── 1. Source lists ────────────────────────────
# PHASE 6: Refactored to use modular kernel/ and drivers/ directories
#
# Legacy sources still in src/ (architecture-specific + not yet migrated)
legacy_src = files(
  'fixed_point.c',
  'fs.c',
  'task.c',            # pure C (TODO: migrate to kernel/sched/)
  'nk_fs.c',
  'context_switch.S',  # hand-written AVR ASM
  'switch_task.S',
  'door_entry.S'
)

# Optional GDB stub
if get_option('debug_gdb')
  legacy_src += files('avr_gdbstub.c')
  add_project_arguments('-DDEBUG_GDB', language : 'c')
endif

# Build full kernel source list from modular components
# NOTE: all_kernel_sources and all_driver_sources come from kernel/ and drivers/
kernel_src = []
kernel_src += all_kernel_sources   # From kernel/ subdirectories (Phase 4)
kernel_src += all_driver_sources   # From drivers/ subdirectories (Phase 5)
kernel_src += legacy_src           # Architecture-specific sources from src/

# Portable subset (no <avr/...>) for host builds
portable_src = []
portable_src += all_kernel_sources
portable_src += all_driver_sources
portable_src += files('fixed_point.c', 'fs.c', 'task.c', 'nk_fs.c')

avr_only_src = []
foreach s : kernel_src
  if not portable_src.contains(s)
    avr_only_src += s
  endif
endforeach

# ───────────────────────── 2. libavrix / libavrix_host ───────────────
# Include paths for modular structure (Phase 4-5)
all_inc = [
  inc,
  include_directories('../kernel/sched'),
  include_directories('../kernel/sync'),
  include_directories('../kernel/mm'),
  include_directories('../kernel/ipc'),
  include_directories('../drivers/fs'),
  include_directories('../drivers/net'),
  include_directories('../drivers/tty'),
]

if meson.is_cross_build()          # ==> target = AVR
  libavrix = static_library(
    'avrix',
    kernel_src,
    include_directories : all_inc,
    install : true
  )
else                                # ==> host build (CI, docs, etc.)
  libavrix = static_library(
    'avrix_host',
    portable_src,
    include_directories : all_inc,
    install : false
  )
  # AVR-only sources are not built on the host
endif

# ───────────────────────── 3. Host-only libfs  ───────────────────────
libfs = static_library(
  'fs',
  'fs.c',
  include_directories : inc,
  native  : true,        # always build for host arch
  install : false
)

# Host-side I/O and EEPROM stubs
if host_machine.cpu_family() != 'avr'
  nk_sim_io = static_library(
    'nk_sim_io',
    'avr_stub.c',
    include_directories : inc,
    native  : true,
    install : false
  )
endif

# ───────────────────────── 4. Public headers  ────────────────────────
install_headers(
  '../include/fixed_point.h',
  '../include/fs.h',
  '../include/nk_lock.h',
  '../include/door.h',
  '../include/memguard.h',
  '../include/nk_task.h',
  '../include/task.h',
  '../include/nk_fs.h',
  '../include/romfs.h',
  '../include/eepfs.h',
  '../include/eeprom_wrap.h',
  '../include/kalloc.h',
  '../include/nk_superlock.h',
  '../include/tty.h',
  '../include/slip.h',
  '../include/ipv4.h',
  '../include/avr_gdbstub.h',
  subdir : 'avrix'
)

# ───────────────────────── 5. Flash helper  ───────────────────────────
# The kernel itself is a static library.  For convenience, convert the
# demo ELF to Intel HEX and invoke ``avrdude``.  This target is gated by
# a cross build and requires ``avr-objcopy`` / ``avrdude`` to be present.

if meson.is_cross_build()
  objcopy = find_program('avr-objcopy', required: false)
  avrdude = find_program('avrdude',    required: false)

  if objcopy.found() and avrdude.found()
    demo_elf = meson.project_build_root() / 'unix0.elf'

    hex_out = custom_target(
      'firmware_hex',
      input  : demo_elf,
      output : 'firmware.hex',
      command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@']
    )

    custom_target(
      'flash',
      input   : hex_out,
      output  : 'flash.stamp',
      command : [
        avrdude,
        '-c', get_option('flash_programmer'),
        '-p', 'm328p',
        '-P', get_option('flash_port'),
        '-U', 'flash:w:@INPUT@'
      ],
      console : true
    )
  endif
endif
