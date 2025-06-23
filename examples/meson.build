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
