# ────────────────────────────────────────────────────────────────────────
#  build_flags/meson.build — central flag definitions (resolved 2025-06-22)
# ------------------------------------------------------------------------
#  • These lists are consumed by every sub-project via:
#        add_project_arguments(common_cflags, language : 'c')
#        add_project_link_arguments(common_ldflags, language : 'c')
#  • A verbatim copy is exported to 'flags.meson' for external tooling.
# ------------------------------------------------------------------------

python = import('python').find_installation('python3')

# Parse the canonical flag lists defined in ``flags.meson`` using Python.
_flag_py = meson.current_source_dir() / 'flags.meson'

common_cflags = run_command(
  python,
  '-c',
  'import pathlib,sys; g={}; exec(open(sys.argv[1]).read(), g); print(" ".join(g["common_cflags"]))',
  _flag_py,
).stdout().strip().split()

common_ldflags = run_command(
  python,
  '-c',
  'import pathlib,sys; g={}; exec(open(sys.argv[1]).read(), g); print(" ".join(g["common_ldflags"]))',
  _flag_py,
).stdout().strip().split()

# ── export for non-Meson tooling ─────────────────────────────────────
# The top-level meson.build serialises these lists to ``flags.meson``
# after appending compiler-specific tweaks (C dialect, POSIX macros).

