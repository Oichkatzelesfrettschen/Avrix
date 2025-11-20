# ─── kernel/sync/meson.build ─────────────────────────────────────────
#
# Synchronization primitives (spinlocks, mutexes)
# ──────────────────────────────────────────────────────────────────────

sync_sources = files(
  'spinlock.c',   # Four-tier spinlock hierarchy (flock/qlock/slock/spinlock)
)

sync_headers = files(
  'spinlock.h',
)

# Export for parent build
sync_dep = declare_dependency(
  sources             : sync_sources,
  include_directories : include_directories('.'),
)
