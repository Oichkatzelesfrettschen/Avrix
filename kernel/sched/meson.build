# ─── kernel/sched/meson.build ────────────────────────────────────────
#
# Portable scheduler (uses HAL for context switching)
# ──────────────────────────────────────────────────────────────────────

sched_sources = files(
  'scheduler.c',   # Round-robin preemptive scheduler
)

sched_headers = files(
  'scheduler.h',
)

# Export for parent build
sched_dep = declare_dependency(
  sources             : sched_sources,
  include_directories : include_directories('.'),
)
