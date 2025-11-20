# ─── kernel/mm/meson.build ───────────────────────────────────────────
#
# Memory management (kalloc heap allocator)
# ──────────────────────────────────────────────────────────────────────

mm_sources = files(
  'kalloc.c',   # Portable kernel heap allocator (bump-pointer + free-list)
)

mm_headers = files(
  'kalloc.h',
)

# Export for parent build
mm_dep = declare_dependency(
  sources             : mm_sources,
  include_directories : include_directories('.'),
)
