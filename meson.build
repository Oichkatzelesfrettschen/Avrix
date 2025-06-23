# ──────────────────────────────────────────────────────────────────────
#  top-level meson.build — µ-UNIX for AVR  (resolved 2025-06-22)
# --------------------------------------------------------------------
#  * Pure C23 (gcc-avr ≥ 7.3, gcc-avr 14.x, clang-20)
#  * Directory layout
#        src/        → kernel + libs (cross)
#        lib/        → header-only helpers
#        examples/   → demos (host-only)
#        tests/      → unit tests (always build, run on host)
#  * Optional docs:  Doxygen (API)  +  Sphinx (manual)
# --------------------------------------------------------------------

project(
  'avrix', 'c',
  version         : '0.1.0',
  license         : 'MIT',
  default_options : [
    'c_std=c2x',
    'warning_level=2',
    'optimization=s',     # -Os for flash size
    'buildtype=release'
  ]
)

# ── import helper modules ─────────────────────────────────────────────
subdir('build')                         # shared flag definitions (if any)
python = import('python').find_installation('python3')

# ── include-path logic ───────────────────────────────────────────────
if meson.is_cross_build() or host_machine.cpu_family() == 'avr'
  inc = include_directories('include')
else
  inc = include_directories('include', 'compat')
endif

# ── optional on-device GDB stub flag ─────────────────────────────────
if get_option('debug_gdb')
  add_project_arguments('-DDEBUG_GDB', language : 'c')
endif

# ── sub-projects ─────────────────────────────────────────────────────
subdir('src')
subdir('examples')
subdir('tests')

# ───────────────────────── documentation targets ─────────────────────
doc_targets = []
doc_doxygen = []

## 1 · Doxygen ########################################################
doxygen = find_program('doxygen', required : false)
if doxygen.found()
  graphviz  = find_program('dot', required : false)

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
    command            : [doxygen, doxy_conf],
    output             : 'docs/doxygen/html/index.html',
    console            : true,
    build_always_stale : true,
    build_by_default   : false
  )
  doc_targets += doc_doxygen
endif

## 2 · Sphinx #########################################################
sphinx = find_program('sphinx-build', required : false)
if sphinx.found()
  doc_sphinx = run_target(
    'doc-sphinx',
    command : [
      sphinx, '-W',
      '-D', 'breathe_projects.avrix=@0@'.format(
        meson.current_source_dir() / 'docs/doxygen/xml'
      ),
      meson.current_source_dir() / 'docs/source',
      meson.current_build_dir()  / 'docs',
      '-a', '-q'
    ],
    depends : doc_doxygen
  )
  doc_targets += doc_sphinx
endif

## 3 · Aggregate target ###############################################
if doc_targets.length() > 0
  run_target('doc', command : ['true'], depends : doc_targets)
endif

# ─────────────────────────── size gate ───────────────────────────────
#   Blocks the build if any firmware ELF exceeds --limit (bytes).
#   Add in meson_options.txt:
#     option('flash_limit', type : 'string', value : '30720',
#            description : 'Maximum allowed firmware size (bytes)')
size_gate = custom_target(
  'size-gate',
  input   : [fs_demo, romfs_demo, slip_demo, ned, vini],
  output  : 'size-gate.stamp',
  command : [
    python,
    files('scripts/size_gate.py'),
    '--limit', get_option('flash_limit'),
    '@INPUT@',      # list of ELF files
    '@OUTPUT@'
  ],
  console : true
)
