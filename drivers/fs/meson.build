# ─── drivers/fs/meson.build ──────────────────────────────────────────
#
# Filesystem drivers (ROMFS, EEPFS, VFS)
# ──────────────────────────────────────────────────────────────────────

fs_driver_sources = files(
  'romfs.c',   # Read-only memory filesystem (flash/ROM)
  'eepfs.c',   # EEPROM filesystem with wear-leveling
  'vfs.c',     # Virtual filesystem layer with mount points
)

fs_driver_headers = files(
  'romfs.h',
  'eepfs.h',
  'vfs.h',
)

# Export for parent build
fs_drivers_dep = declare_dependency(
  sources             : fs_driver_sources,
  include_directories : include_directories('.'),
)
