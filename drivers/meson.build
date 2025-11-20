# ─── drivers/meson.build ─────────────────────────────────────────────
#
# Driver layer - Portable device drivers
# ──────────────────────────────────────────────────────────────────────
# Phase 5 refactoring: Extracted all drivers from src/ to drivers/
#
# Structure:
#   drivers/fs/    - Filesystem drivers (ROMFS, EEPFS, VFS)
#   drivers/net/   - Networking drivers (SLIP, IPv4)
#   drivers/tty/   - Character device drivers (TTY)
# ──────────────────────────────────────────────────────────────────────

# Include subdirectories
subdir('fs')
subdir('net')
subdir('tty')

# Aggregate all driver sources
all_driver_sources = []
all_driver_sources += fs_driver_sources
all_driver_sources += net_driver_sources
all_driver_sources += tty_driver_sources

# Export combined dependency
drivers_dep = declare_dependency(
  sources             : all_driver_sources,
  include_directories : [
    include_directories('fs'),
    include_directories('net'),
    include_directories('tty'),
  ],
)
