# ─── tests/meson.build ────────────────────────────────────────────────
#
#  • Native build   → link tests against **libavrix** (full host stub).
#  • Cross-build    → build test OBJs for *host* CPU (`native:true`)
#                     and link only against the portable **libfs** helper lib.
#
#  • `check_docs.py` always runs; it fails if any Sphinx :toctree: entry
#    points to a missing file.
#
#  • Extra host toggles (meson_options.txt):
#        -Dsan=yes   → Address-/UB-sanitiser
#        -Dcov=yes   → llvm-coverage flags
#        -Davr_inc_dir=/path → custom AVR include dir for host headers
#
#  Imported from the top-level meson.build:
#        inc        include_directories('include')
#        libavrix   host stub (native only)
#        libfs      portable RAM-filesystem (always)
# --------------------------------------------------------------------

python_mod = import('python')
python     = python_mod.find_installation('python3')
fs_mod     = import('fs')

# ───────── 0 · Sphinx :toctree: link checker ─────────────────────────
test(
  'check-docs',
  python,
  args : [
    files('check_docs.py'),
    '--docs-dir',  meson.project_source_root() / 'docs/source',
    '--index-file', meson.project_source_root() / 'docs/source/index.rst',
    '--recursive',
  ],
  workdir : meson.project_source_root()
)

# ───────── 1 · Include-path negotiation (host vs AVR) ───────────────
inc_list = [inc]

if host_machine.cpu_family() != 'avr'
  inc_list += include_directories('../compat')   # stub <avr/...>
endif

opt_avr_inc = get_option('avr_inc_dir')
if opt_avr_inc != '' and fs_mod.is_dir(opt_avr_inc)
  inc_list += include_directories(opt_avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# ───────── 2 · Compiler flags, sanitiser & coverage ------------------
common_cflags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17']

if not meson.is_cross_build()
  if get_option('san') == 'yes' and cc.get_id() in ['gcc', 'clang']
    san_flags = ['-fsanitize=address,undefined']
    common_cflags += san_flags
    add_project_arguments(san_flags, language : 'c')
  endif

  if get_option('cov') == 'yes' and cc.has_argument('-fprofile-instr-generate')
    cov_flags = ['-fprofile-instr-generate', '-fcoverage-mapping']
    common_cflags += cov_flags
    add_project_arguments(cov_flags, language : 'c')
  endif
endif

# compile tests **always** for host CPU
build_test = (name : str, sources : list, link_target) =>
  executable(
    name,
    sources,
    include_directories : inc_list,
    link_with            : link_target,
    c_args               : common_cflags,
    native               : true
  )

# Host builds need a replacement for <avr/io.h>
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

link_full = meson.is_cross_build() ? libfs : libavrix

# ───────── 3 · Core unit tests --------------------------------------
fixed_point_impl = meson.project_source_root() / 'src/fixed_point.c'
fixed_point_blob = [fixed_point_impl] if fs_mod.exists(fixed_point_impl) else []

tests = [
  ['test_fixed_point', fixed_point_blob + ['test_fixed_point.c']],
  ['fs_test',          ['fs_test.c',        'sim.c']],
  ['flock_stress',     ['flock_stress.c',   'sim.c']],
  ['spin_test',        ['spin_test.c',      'sim.c']],
  ['superlock_test',   ['superlock_test.c', 'sim.c']],
  ['romfs_test',       ['romfs_test.c',     'sim.c']],
  ['fs_roundtrip',     ['fs_roundtrip.c',   'sim.c']],
]

foreach t : tests
  exe = build_test(t.get(0), t.get(1) + extra_src, link_full)
  test(t.get(0), exe)
endforeach

# ───────── 4 · simavr smoke tests (only if cross-compiling) ----------
if target_machine.cpu_family() == 'avr'
  simavr = find_program('simavr', required : false)
  if simavr.found()
    sim_suites = [
      ['fs_simavr_basic',
        ['fs_simavr_basic.c', meson.project_source_root() / 'src/fs.c']],
      ['spinlock_isr',
        ['spinlock_isr.c',    meson.project_source_root() / 'src/nk_lock.c']],
    ]

    foreach s : sim_suites
      sim_exe = executable(
        s.get(0),
        s.get(1) + extra_src,
        include_directories : inc_list,
        c_args              : ['-Os', '-std=c11', '-Wall', '-Wextra', '-pedantic']
      )

      test(
        s.get(0),
        simavr,
        args        : ['-m', 'atmega328p', sim_exe],
        is_parallel : false,
        timeout     : 30
      )
    endforeach
  endif
endif
