# ─── drivers/tty/meson.build ─────────────────────────────────────────
#
# TTY (Teletype) character device driver
# ──────────────────────────────────────────────────────────────────────

tty_driver_sources = files(
  'tty.c',    # Ring buffer based TTY with fast modulo optimization
)

tty_driver_headers = files(
  'tty.h',
)

# Export for parent build
tty_drivers_dep = declare_dependency(
  sources             : tty_driver_sources,
  include_directories : include_directories('.'),
)
