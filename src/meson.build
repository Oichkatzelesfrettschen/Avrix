# ─── src/meson.build ────────────────────────────────────────────────
# If we are cross-compiling for AVR, pull in every kernel module.
# For a native host build (unit-tests, doc generation) compile only
# the portable helpers that do not include <avr/io.h>.
# -------------------------------------------------------------------

if meson.is_cross_build()
  sources = files(
    'fixed_point.c',
    'fs.c',
    'door.c',
    'task.c',
    'context_switch.S',
    'nk_fs.c',
    'kalloc.c'
  )
else
  sources = files(
    'fixed_point.c',
    'fs.c'
  )
endif

libavrix = static_library(
  'avrix',
  sources,
  include_directories : inc,
  install : true
)

install_headers(
  '../include/fixed_point.h',
  '../include/fs.h',
  '../include/nk_lock.h',
  '../include/door.h',
  '../include/memguard.h',
  '../include/nk_task.h',
  '../include/nk_fs.h',
  '../include/kalloc.h',
  subdir : 'avrix'
)
