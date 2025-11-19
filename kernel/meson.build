# ─── kernel/meson.build ──────────────────────────────────────────────
#
# Portable kernel subsystems
# ──────────────────────────────────────────────────────────────────────
# Phase 4 refactoring: Extracted kernel subsystems from src/
#
# Structure:
#   kernel/sched/  - Scheduler (portable, uses HAL for context switching)
#   kernel/sync/   - Synchronization primitives (spinlocks, mutexes)
#   kernel/mm/     - Memory management (kalloc heap allocator)
#   kernel/ipc/    - Inter-process communication (Door RPC)
# ──────────────────────────────────────────────────────────────────────

# Include subdirectories
subdir('sched')
subdir('sync')
subdir('mm')
subdir('ipc')

# Aggregate all kernel sources
all_kernel_sources = []
all_kernel_sources += sched_sources
all_kernel_sources += sync_sources
all_kernel_sources += mm_sources
all_kernel_sources += ipc_sources

# Export combined dependency
kernel_dep = declare_dependency(
  sources             : all_kernel_sources,
  include_directories : [
    include_directories('sched'),
    include_directories('sync'),
    include_directories('mm'),
    include_directories('ipc'),
  ],
)
