# ────────────────────────────────────────────────────────────────────────
#  build_flags/meson.build  —  authoritative flag oracle  (22 Jun 2025)
# -----------------------------------------------------------------------
#  • Define base optimisation / warning flags **only here**.
#  • NO language-dialect flags: the top-level meson.build probes the
#    compiler and appends -std=c23 / -std=c2x / -std=c11 as appropriate.
#  • Emit a Meson fragment (flags.meson) for IDE & Docker tooling via
#    configure_file() – the canonical Meson idiom. :contentReference[oaicite:1]{index=1}
# -----------------------------------------------------------------------

common_cflags = [
  '-pipe',
  '-ffunction-sections', '-fdata-sections', '-fno-common',
  '-Os',                     # favour flash / ROM footprint
  '-Wall', '-Wextra', '-Wpedantic',
]

common_ldflags = [
  '-Wl,--gc-sections',       # dead-strip unused .text/.data
]

# ── 1 · export snippet for external tools ------------------------------------
conf = configuration_data()                          # :contentReference[oaicite:2]{index=2}
conf.set('COMMON_CFLAGS',
         '[' + ', '.join("'" + f + "'" for f in common_cflags) + ']')
conf.set('COMMON_LDFLAGS',
         '[' + ', '.join("'" + f + "'" for f in common_ldflags) + ']')

configure_file(                                      # writes flags.meson
  input        : 'flags.meson.in',
  output       : 'flags.meson',
  configuration: conf,
)
