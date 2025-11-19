# ──────────────── demo executables ────────────────────────────────────

fs_demo = executable(
  'fs_demo',
  'fs_demo.c',
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : common_cflags,
  install             : false
)

romfs_demo = executable(
  'romfs_demo',
  'romfs_demo.c',
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : common_cflags,
  install             : false
)

slip_demo = executable(
  'slip_demo',
  'slip_demo.c',
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : common_cflags,
  install             : false
)

# ────────── ELF → HEX conversion for simavr/avrdude ──────────────────
objcopy = find_program('avr-objcopy', required : false)
if objcopy.found()
  custom_target('fs_demo_hex',
    command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input   : fs_demo,
    output  : 'fs_demo.hex'
  )
  custom_target('romfs_demo_hex',
    command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input   : romfs_demo,
    output  : 'romfs_demo.hex'
  )
  custom_target('slip_demo_hex',
    command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input   : slip_demo,
    output  : 'slip_demo.hex'
  )
endif

# ─────────────── optimisation flags (shared) ─────────────────────────
opt_cflags = common_cflags + ['-Os']
if host_machine.cpu_family() == 'avr'
  opt_cflags += ['-fno-stack-protector']
endif

# ───────────── editor demos (ned / vini) ─────────────────────────────
ned = executable(
  'ned',
  ['ned.c', 'editor_common.c'],
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : opt_cflags + ['-DMAX_LINES=16', '-DMAX_LINE_LEN=64'],
  install             : false
)

vini = executable(
  'vini',
  ['vini.c', 'editor_common.c'],
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : opt_cflags + ['-DMAX_LINES=14', '-DMAX_LINE_LEN=64'],
  install             : false
)

# ─────────────── door RPC host-only demo ──────────────────────────────
if not meson.is_cross_build()
  door_demo = executable(
    'door_demo',
    'door_demo.c',
    include_directories : inc,
    c_args              : common_cflags,
    native              : true,
    install             : false
  )
endif

# ═══════════════════════════════════════════════════════════════════
# PHASE 7: POSIX PROFILE EXAMPLES (Tier-Based)
# ═══════════════════════════════════════════════════════════════════
#
# PSE51 (low_tier):  Minimal POSIX for 8-16KB flash MCUs
# PSE52 (mid_tier):  Multi-threaded POSIX for 128KB flash MCUs
# PSE54 (high_tier): Full POSIX for ARM Cortex-A, RISC-V with MMU
# ═══════════════════════════════════════════════════════════════════

# ─────────────── PSE51: Low-Tier Examples (Minimal Profile) ──────────
# Target: ATmega128, 8-16KB flash, 1-2KB RAM
# Profile: Single-threaded, no IPC, basic I/O

if not meson.is_cross_build()
  # PSE51 examples require native/host build (stdio)

  foreach example : ['hello_pse51', 'romfs_pse51', 'eeprom_pse51', 'scheduler_pse51']
    executable(
      example,
      'low_tier' / example + '.c',
      include_directories : inc,
      link_with           : [libavrix, nk_sim_io],
      c_args              : common_cflags,
      native              : true,
      install             : false
    )
  endforeach
endif

# ─────────────── PSE52: Mid-Tier Examples (Multi-Threaded) ───────────
# Target: ATmega1284, 128KB flash, 16KB RAM
# Profile: pthread, networking, IPC

if not meson.is_cross_build()
  foreach example : ['threading_pse52', 'network_pse52', 'ipc_pse52', 'vfs_pse52']
    executable(
      example,
      'mid_tier' / example + '.c',
      include_directories : inc,
      link_with           : [libavrix, nk_sim_io],
      c_args              : common_cflags,
      native              : true,
      install             : false
    )
  endforeach
endif

# ─────────────── PSE54: High-Tier Examples (Full POSIX) ──────────────
# Target: ARM Cortex-A, RISC-V with MMU
# Profile: Processes, signals, MMU, full IPC

if not meson.is_cross_build()
  foreach example : ['signals_pse54', 'processes_pse54', 'mmu_pse54', 'full_posix_pse54']
    executable(
      example,
      'high_tier' / example + '.c',
      include_directories : inc,
      c_args              : common_cflags,
      native              : true,
      install             : false,
      # PSE54 examples use standard POSIX (no embedded-specific linking)
    )
  endforeach
endif
