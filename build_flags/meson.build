# -----------------------------------------------------------------------------
#  build_flags/meson.build
#  Authoritative flag oracle for Avrix (µ-UNIX) ― **Do Not Duplicate**
#  2025-06-22
#
#  • Define base optimisation / warning flags here only.
#  • No language-dialect flag lives here; the top-level `meson.build`
#    probes the compiler and appends `-std=c23 / c2x / c11` as needed.
#  • A Meson fragment `flags.meson` is emitted for IDE, Docker, and
#    external tooling via `configure_file()` (canonical Meson idiom).
#  • This file is the single source of truth; all sub-projects include it
#    with `subdir('build_flags')` and inherit `common_cflags` /
#    `common_ldflags`.  Never edit the generated `flags.meson`.
# -----------------------------------------------------------------------------

# ── 0 · Canonical compiler & linker flags ───────────────────────────────
common_cflags = [
  '-pipe',
  '-ffunction-sections', '-fdata-sections', '-fno-common',
  '-Os',                      # favour flash / ROM footprint
  '-Wall', '-Wextra', '-Wpedantic',
  # NOTE: dialect flag appended in top-level meson.build
]

common_ldflags = [
  '-Wl,--gc-sections',        # dead-strip unused .text/.data
]

# Generation of flags.meson is handled by scripts/gen_flags.py in the
# top-level meson.build.  Keeping logic here minimal avoids Meson
# string-manipulation pitfalls.
