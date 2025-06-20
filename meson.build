# ──────────────────────────────────────────────────────────────────────
#  top-level meson.build  —  µ-UNIX for AVR
# --------------------------------------------------------------------
#  * Pure C23 code (but builds with gcc-avr ≥ 7.3, clang-20, gcc-avr 14)
#  * Each subdir provides its own Meson file:
#        src/        → kernel + libs for the cross build
#        lib/        → third-party helpers (header-only, etc.)
#        examples/   → host-side demos / sims (built when NOT cross)
#        tests/      → native unit tests (always compiled; only run natively)
#  * Optional documentation: Doxygen (API) + Sphinx (manual)
# --------------------------------------------------------------------

project(
  'avrix', 'c',
  version          : '0.1.0',
  license          : 'MIT',
  default_options  : [
    'c_std=c23',
    'warning_level=2',
    'optimization=z',      # maps to -Oz
    'buildtype=release'
  ]
)

inc = include_directories('include')

subdir('src')
subdir('lib')
subdir('examples')
subdir('tests')

# ─────────────────────────  Documentation  ───────────────────────────
# Build only if the tools exist; aggregate into a single `meson doc`.

doc_targets = []

## 1.  Doxygen (API reference) ########################################
doxygen = find_program('doxygen', required : false)
if doxygen.found()
  doc_targets += run_target(
    'doc-doxygen',
    command : [
      'sh', '-c',
      'cd @0@ && @1@ Doxyfile'.format(meson.current_source_dir(), doxygen.path())
    ]
  )
endif

## 2.  Sphinx (User manual) ###########################################
sphinx = find_program('sphinx-build', required : false)
if sphinx.found()
  doc_targets += run_target(
    'doc-sphinx',
    command : [
      sphinx,
      meson.current_source_dir() / 'docs/source',   # input dir
      meson.current_build_dir()  / 'docs',          # output dir
      '-a', '-q'                                    # rebuild all, quiet
    ]
  )
endif

## 3.  Aggregator ######################################################
if doc_targets.length() > 0
  alias_target('doc', doc_targets)   # `meson compile doc` builds all docs
endif
