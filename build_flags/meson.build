# ────────────────────────────────────────────────────────────────────────
#  build_flags/meson.build — central flag definitions (source of truth)
# ------------------------------------------------------------------------
#  • Consumed by every sub-project via ``common_cflags`` and ``common_ldflags``.
#  • ``flags.meson`` is generated from this file for external tooling.
# ------------------------------------------------------------------------

# Canonical compiler and linker flag lists
common_cflags = [
  '-pipe',
  '-ffunction-sections',
  '-fdata-sections',
  '-fno-common',
  '-Os',            # size-optimised; retain -O2 for perf-critical objs
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  # The C dialect flag is appended by the top-level meson.build
]

common_ldflags = [
  '-Wl,--gc-sections',   # dead-strip unused .text/.data
]

# ── export for non-Meson tooling ─────────────────────────────────────
# ``flags.meson`` mirrors these lists verbatim.
flag_conf = configuration_data()
flag_conf.set('COMMON_CFLAGS', '@0@'.format(common_cflags))
flag_conf.set('COMMON_LDFLAGS', '@0@'.format(common_ldflags))

configure_file(
  input  : 'flags.meson.in',
  output : 'flags.meson',
  configuration : flag_conf,
)

