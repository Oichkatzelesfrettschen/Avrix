# ─── drivers/fs/meson.build ──────────────────────────────────────────
#
# Filesystem drivers (ROMFS, EEPFS, VFS)
# ──────────────────────────────────────────────────────────────────────

fs_driver_sources = files('vfs.c') # VFS is always needed if fs_enabled

if get_option('fs_romfs_enabled')
  fs_driver_sources += files('romfs.c')
endif

if get_option('fs_eepfs_enabled')
  fs_driver_sources += files('eepfs.c')
endif

# Export for parent build
fs_drivers_dep = declare_dependency(
  sources             : fs_driver_sources,
  include_directories : include_directories('.'),
)
