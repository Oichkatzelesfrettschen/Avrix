# ──────────────── demo executables ────────────────────────────────────

# Legacy demos (guarded by features)
if get_option('fs_enabled')
  fs_demo = executable(
    'fs_demo',
    'fs_demo.c',
    include_directories : inc,
    link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
    c_args              : common_cflags,
    install             : false
  )
endif

if get_option('fs_romfs_enabled')
  romfs_demo = executable(
    'romfs_demo',
    'romfs_demo.c',
    include_directories : inc,
    link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
    c_args              : common_cflags,
    install             : false
  )
endif

if get_option('net_slip_enabled')
  slip_demo = executable(
    'slip_demo',
    'slip_demo.c',
    include_directories : inc,
    link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
    c_args              : common_cflags,
    install             : false
  )
endif

# ────────── ELF → HEX conversion for simavr/avrdude ──────────────────
objcopy = find_program('avr-objcopy', required : false)
if objcopy.found()
  if get_option('fs_enabled')
    custom_target('fs_demo_hex',
      command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
      input   : fs_demo,
      output  : 'fs_demo.hex'
    )
  endif
  if get_option('fs_romfs_enabled')
    custom_target('romfs_demo_hex',
      command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
      input   : romfs_demo,
      output  : 'romfs_demo.hex'
    )
  endif
  if get_option('net_slip_enabled')
    custom_target('slip_demo_hex',
      command : [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
      input   : slip_demo,
      output  : 'slip_demo.hex'
    )
  endif
endif

# ─────────────── optimisation flags (shared) ─────────────────────────
opt_cflags = common_cflags + ['-Os']
if host_machine.cpu_family() == 'avr'
  opt_cflags += ['-fno-stack-protector']
endif

# ───────────── editor demos (ned / vini) ─────────────────────────────
# Guards for editor demos? They might use FS
ned = executable(
  'ned',
  ['ned.c', 'editor_common.c'],
  include_directories : inc,
  link_with           : [libavrix] + (meson.is_cross_build() ? [] : [nk_sim_io]),
  c_args              : opt_cflags + ['-DMAX_LINES=16', '-DMAX_LINE_LEN=64'],
  install             : false
)

# vini uses termios.h (POSIX terminal I/O) - host-only
if not meson.is_cross_build()
  vini = executable(
    'vini',
    ['vini.c', 'editor_common.c'],
    include_directories : inc,
    link_with           : [libavrix, nk_sim_io],
    c_args              : opt_cflags + ['-DMAX_LINES=14', '-DMAX_LINE_LEN=64'],
    native              : true,
    install             : false
  )
else
  # Create dummy target for cross-builds (size-gate expects vini to exist)
  vini = custom_target('vini_dummy',
    output : 'vini.dummy',
    command : ['touch', '@OUTPUT@']
  )
endif

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

# ─────────────── PSE51: Low-Tier Examples (Minimal Profile) ──────────
if not meson.is_cross_build()
  # Only include basic examples for Low Tier
  low_examples = ['hello_pse51', 'scheduler_pse51']

  # eeprom_pse51 requires EEPFS
  if get_option('fs_eepfs_enabled')
    low_examples += ['eeprom_pse51']
  endif

  # romfs_pse51 requires ROMFS
  if get_option('fs_romfs_enabled')
    low_examples += ['romfs_pse51']
  endif

  foreach example : low_examples
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
if not meson.is_cross_build()
  mid_examples = ['threading_pse52', 'ipc_pse52']

  if get_option('net_enabled')
    mid_examples += ['network_pse52']
  endif

  if get_option('fs_enabled')
    mid_examples += ['vfs_pse52']
  endif

  foreach example : mid_examples
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
# Note: High tier examples assume High Profile features
if not meson.is_cross_build()
  # Build unconditionally as they use standard POSIX headers (host OS)
  # but strict embedded logic might need checks.
  foreach example : ['signals_pse54', 'processes_pse54', 'mmu_pse54', 'full_posix_pse54']
    executable(
      example,
      'high_tier' / example + '.c',
      include_directories : inc,
      c_args              : common_cflags,
      native              : true,
      install             : false
    )
  endforeach
endif
