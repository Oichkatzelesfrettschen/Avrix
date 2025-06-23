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
kernel_src = files(
  'fixed_point.c',
  'fs.c',
  'door.c',
  'task.c',            # pure C
  'nk_fs.c',
  'romfs.c',
  'eepfs.c',
  'kalloc.c',
  'nk_superlock.c',
  'tty.c',
  'slip_uart.c',
  'ipv4.c',
  'context_switch.S',  # hand-written AVR ASM
  'switch_task.S',
  'door_entry.S'
)

if get_option('debug_gdb')
  kernel_src += files('gdbstub.c')
endif

portable_src = files(   # no <avr/...>
  'fixed_point.c',
  'fs.c',
  'romfs.c',
  'eepfs.c',
  'nk_superlock.c',
  'tty.c',
  'slip_uart.c',
  'ipv4.c'
)

avr_only_src = []
foreach s : kernel_src
  if not portable_src.contains(s)
    avr_only_src += s
  endif
endforeach

# ───────────────────────── 2. libavrix / libavrix_host ───────────────
if meson.is_cross_build()          # ==> target = AVR
  libavrix = static_library(
    'avrix',
    kernel_src,
    include_directories : inc,
    install : true
  )
else                                # ==> host build (CI, docs, etc.)
  libavrix = static_library(
    'avrix_host',
    portable_src,
    include_directories : inc,
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
  '../include/gdbstub.h',
  subdir : 'avrix'
)

# ───────────────────────── 5. Flash helper  ───────────────────────────
# The kernel itself is a static library.  For convenience, convert the
# demo ELF to Intel HEX and invoke ``avrdude``.  This target is gated by
# a cross build and requires ``avr-objcopy`` / ``avrdude`` to be present.

if meson.is_cross_build()
  objcopy = find_program('avr-objcopy', required: false)
  avrdude = find_program('avrdude', required: false)

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
      input  : hex_out,
      output : 'flash.stamp',
      command: [
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
