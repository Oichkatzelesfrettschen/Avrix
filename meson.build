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
    'c_std=c2x',
    'warning_level=2',
    'optimization=s',      # -Os for size optimizations
    'buildtype=release'
  ]
)

if host_machine.cpu_family() != 'avr'
  inc = include_directories('include', 'compat')
else
  inc = include_directories('include')
endif

subdir('src')
subdir('examples')
subdir('tests')

# ─────────────────────────  Documentation  ───────────────────────────
# Build only if the tools exist; aggregate into a single `meson doc`.

doc_targets = []
doc_doxygen = []

## 1.  Doxygen (API reference) ########################################
doxygen = find_program('doxygen', required : false)
if doxygen.found()

doc_doxygen = custom_target(
    'doc-doxygen',
    input  : 'Doxyfile',
    output : 'doxygen.stamp',
    command : [doxygen, '@INPUT@'],
    console : true,
    build_always_stale : true,
    build_by_default : false,
)

doxy_conf = configure_file(
    input  : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : {
      'OUTPUT_DIR' : meson.current_build_dir() / 'docs/doxygen'
    }
  )
  doc_doxygen = custom_target(
    'doc-doxygen',
    command         : [doxygen, doxy_conf],
    output          : 'docs/doxygen/html/index.html',
    build_by_default: false

  )
  doc_targets += doc_doxygen
endif

## 2.  Sphinx (User manual) ###########################################
sphinx = find_program('sphinx-build', required : false)
if sphinx.found()
  doc_sphinx = run_target(
    'doc-sphinx',
    command : [
      sphinx,
      '-D',
      'breathe_projects.avrix=@0@'.format(
        meson.current_source_dir() / 'docs/doxygen/xml'
      ),
      meson.current_source_dir() / 'docs/source',   # input dir
      meson.current_build_dir()  / 'docs',          # output dir
      '-a', '-q'                                    # rebuild all, quiet
    ],
    depends : doc_doxygen
  )
  doc_targets += doc_sphinx
endif

## 3.  Aggregator ######################################################
if doc_targets.length() > 0
  alias_target('doc', doc_targets)   # `meson compile doc` builds all docs
endif
