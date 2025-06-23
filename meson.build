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

python = import('python').find_installation('python3')

## Include directories
#
# Host builds should use compatibility headers that emulate the AVR
# environment.  Cross builds for AVR rely on the real toolchain
# headers instead.  This prevents host-only builds from requiring an
# AVR toolchain while ensuring cross builds are unaffected.
if meson.is_cross_build() or host_machine.cpu_family() == 'avr'
  inc = include_directories('include')
else
  inc = include_directories('include', 'compat')
endif

# Optional on-device GDB stub
if get_option('debug_gdb')
  add_project_arguments('-DDEBUG_GDB', language : 'c')
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

  graphviz = find_program('dot', required : false)

  doxy_conf = configure_file(
    input  : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : {
      'OUTPUT_DIR' : meson.current_build_dir() / 'docs/doxygen',
      'HAVE_DOT'   : graphviz.found() ? 'YES' : 'NO'
    }
  )

  doc_doxygen = custom_target(
    'doc-doxygen',
    command           : [doxygen, doxy_conf],
    output            : 'docs/doxygen/html/index.html',
    console           : true,
    build_always_stale: true,
    build_by_default  : false,
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
      '-W',  # treat warnings as errors
      '-D',
      'breathe_projects.avrix=@0@'.format(
        meson.current_source_dir() / 'docs/doxygen/xml'
      ),
      meson.current_source_dir() / 'docs/source',   # input dir
      meson.current_build_dir()  / 'docs',          # output dir
      '-a', '-q'
    ],
    depends : doc_doxygen
  )
  doc_targets += doc_sphinx
endif

## 3.  Aggregator ######################################################
if doc_targets.length() > 0
  run_target('doc', command : ['true'], depends : doc_targets)
endif

# ─────────────────────────  Size gate  ─────────────────────────────────
size_gate = custom_target(
  'size-gate',
  input  : [fs_demo, romfs_demo, slip_demo, ned, vini],
  output : 'size-gate.stamp',
  command: [
    python, files('scripts/size_gate.py'),
    '@INPUT@',
    '30720',
    '@OUTPUT@'
  ],
  console : true
)
