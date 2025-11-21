# ─── drivers/meson.build ─────────────────────────────────────────────
#
# Driver layer - Portable device drivers
# ──────────────────────────────────────────────────────────────────────

all_driver_sources = []
drivers_inc = []

# ── Filesystem ──
if get_option('fs_enabled')
  subdir('fs')
  all_driver_sources += fs_driver_sources
  drivers_inc += include_directories('fs')
else
  # Even if disabled, we might want to include headers for stubs if they are not in include/
  # But our stubs are in include/fs.h, so this is fine.
  fs_driver_sources = []
endif

# ── Networking ──
if get_option('net_enabled')
  subdir('net')
  all_driver_sources += net_driver_sources
  drivers_inc += include_directories('net')
else
  net_driver_sources = []
endif

# ── TTY ──
if get_option('tty_enabled')
  subdir('tty')
  all_driver_sources += tty_driver_sources
  drivers_inc += include_directories('tty')
else
  tty_driver_sources = []
endif

# Export combined dependency
drivers_dep = declare_dependency(
  sources             : all_driver_sources,
  include_directories : drivers_inc,
)
