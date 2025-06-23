# ──────────────────────────────────────────────────────────────────────
#  top-level meson.build — µ-UNIX for AVR   (22 Jun 2025, v4.0)
# ---------------------------------------------------------------------
#  * Tool-chains      :   gcc-avr ≥ 7.3, gcc-avr 14.x, clang-20+
#  * Directory layout :   src/ lib/ examples/ tests/
#  * Docs (optional)  :   Doxygen  +  Sphinx
# ---------------------------------------------------------------------

project(
  'avrix', 'c',
  version         : '0.1.0',
  license         : 'MIT',
  default_options : [
    'c_std=none',          # dialect decided below via cc.has_argument()
    'warning_level=2',
    'optimization=s',      # -Os  (flash-oriented)
    'buildtype=release',
  ],
)

# ── 1 · import base flag lists (no -std yet) ──────────────────────────
subdir('build_flags')                    # sets common_cflags / common_ldflags

# ── 2 · compiler dialect & Clang POSIX helpers ────────────────────────
cc             = meson.get_compiler('c')
std_candidates = ['-std=c23', '-std=c2x', '-std=c11']
std_flag       = []
foreach cand : std_candidates
  if cc.has_argument(cand)
    std_flag = [cand]
    break
  endif
endforeach

posix_flag = []
if cc.get_id() == 'clang' and host_machine.cpu_family() != 'avr'
  posix_flag = ['-D_POSIX_C_SOURCE=200809L', '-D_GNU_SOURCE']
endif

common_cflags += std_flag + posix_flag

# ── 3 · propagate flags project-wide ──────────────────────────────────
add_project_arguments(common_cflags, language : 'c')
add_project_link_arguments(common_ldflags, language : 'c')

# ── 4 · export finalised flag lists for external tools  ───────────────
python = import('python').find_installation('python3')
run_command(
  python,
  files('scripts/gen_flags.py'),
  meson.current_source_dir() / 'build_flags/flags.meson',
  ';'.join(common_cflags),          # stringify for the helper script
  ';'.join(common_ldflags),
  check : true,
)

# ── 5 · include-path selection ────────────────────────────────────────
if meson.is_cross_build() or host_machine.cpu_family() == 'avr'
  inc = include_directories('include')
else
  inc = include_directories('include', 'compat')
endif

# ── 6 · optional on-device GDB stub -----------------------------------
if get_option('debug_gdb')
  add_project_arguments('-DDEBUG_GDB', language : 'c')
endif

# ── 7 · sub-projects ---------------------------------------------------
subdir('src')
subdir('examples')
subdir('tests')

# ── 8 · documentation (Doxygen + Sphinx) -------------------------------
doc_targets  = []
doc_doxygen  = []

doxygen = find_program('doxygen', required : false)
if doxygen.found()
  graphviz = find_program('dot', required : false)
  doxy_conf = configure_file(
    input         : 'Doxyfile.in',
    output        : 'Doxyfile',
    configuration : {
      'OUTPUT_DIR' : meson.current_build_dir() / 'docs/doxygen',
      'HAVE_DOT'   : graphviz.found() ? 'YES' : 'NO',
    },
  )
  doc_doxygen = custom_target(
    'doc-doxygen',
    command            : [doxygen, doxy_conf],
    output             : 'docs/doxygen/html/index.html',
    console            : true,
    build_always_stale : true,
    build_by_default   : false,
  )
  doc_targets += doc_doxygen
endif

sphinx = find_program('sphinx-build', required : false)
if sphinx.found()
  doc_sphinx = run_target(
    'doc-sphinx',
    command : [
      sphinx, '-W',
      '-D', 'breathe_projects.avrix=@0@'
            .format(meson.current_source_dir() / 'docs/doxygen/xml'),
      meson.current_source_dir() / 'docs/source',
      meson.current_build_dir()  / 'docs',
      '-a', '-q',
    ],
    depends : doc_doxygen,
  )
  doc_targets += doc_sphinx
endif

if doc_targets.length() > 0
  run_target('doc', command : ['true'], depends : doc_targets)
endif

# ── 9 · firmware size-gate (optional) ---------------------------------
if meson.is_cross_build() and get_option('flash_limit')
  gate_cmd = [
    python,
    files('scripts/size_gate.py'),
    '--limit', get_option('flash_limit_bytes'),
    '@INPUT@', '@OUTPUT@',
  ]
else
  gate_cmd = ['true']
endif

size_gate = custom_target(
  'size-gate',
  input   : [fs_demo, romfs_demo, slip_demo, ned, vini,
             files('scripts/size_gate.py')],
  output  : 'size-gate.stamp',
  command : gate_cmd,
  console : true,
)
