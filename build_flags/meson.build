# ────────────────────────────────────────────────────────────────────────
#  build_flags/meson.build — central flag definitions (resolved 2025-06-22)
# ------------------------------------------------------------------------
#  • These lists are consumed by every sub-project via:
#        add_project_arguments(common_cflags, language : 'c')
#        add_project_link_arguments(common_ldflags, language : 'c')
#  • A verbatim copy is exported to 'flags.meson' for external tooling.
# ------------------------------------------------------------------------

common_cflags = [
  '-pipe',
  '-ffunction-sections',
  '-fdata-sections',
  '-fno-common',
  '-Os',            # size-optimised, retain -O2 for perf-critical objs
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-std=c23',       # align with top-level `c_std=c23`; supersedes draft c2x
]

common_ldflags = [
  '-Wl,--gc-sections',   # dead-strip unused .text/.data
]
