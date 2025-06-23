# ──────────── tests/meson.build ────────────────────────────────────────
# Native build   → links tests against **libavrix_host** stub.
# Cross build    → compiles tests for the host CPU (`native : true`)
#                  and links only against portable **libfs** helpers.
#
# Options (meson_options.txt):
#   -Dsan=true      → Address & UB sanitizers (host only)
#   -Dcov=true      → LLVM coverage flags     (host only)
#   -Davr_inc_dir=… → Additional AVR header path
#
# Imported from root meson.build:
#   inc       → include_directories('include')
#   libavrix  → host stub   (native builds)
#   libfs     → portable RAM-FS helpers
#   nk_sim_io → I/O shim for host simulations
# -----------------------------------------------------------------------

python   = import('python').find_installation('python3')
fs_mod   = import('fs')
cc       = meson.get_compiler('c')

san      = get_option('san')
cov      = get_option('cov')

# ───────────────────── 0 · Documentation lint  ────────────────────────
test(
  'check-docs',
  python,
  args : [
    files('check_docs.py'),
    '--docs-dir',   meson.current_source_dir() / '..' / 'docs' / 'source',
    '--index-file', meson.current_source_dir() / '..' / 'docs' / 'source' /
                     'index.rst',
    '--recursive',
  ],
  workdir : meson.project_source_root()
)

# ───────────────────── 1 · Include-path assembly  ─────────────────────
inc_list = [inc]

# Host builds need AVR-compat headers
if host_machine.cpu_family() != 'avr'
  inc_list += include_directories('..' / 'compat')
endif

avr_inc = get_option('avr_inc_dir')
if avr_inc != '' and fs_mod.is_dir(avr_inc)
  inc_list += include_directories(avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# ───────────────────── 2 · Common compiler flags  ─────────────────────
# Start from project-wide defaults defined in build/flags.meson
test_cflags  = common_cflags
test_ldflags = common_ldflags

if not meson.is_cross_build()
  if san and cc.get_id() in ['gcc', 'clang']
    test_cflags += ['-fsanitize=address,undefined']
    test_ldflags += ['-fsanitize=address,undefined']
  endif

  if cov and cc.has_argument('-fcoverage-mapping')
    test_cflags += ['-fprofile-instr-generate', '-fcoverage-mapping']
    test_ldflags += ['-fprofile-instr-generate', '-fcoverage-mapping']
  endif
endif

# ───────────────────── 3 · Link-target selection  ─────────────────────
extra_libs  = (host_machine.cpu_family() != 'avr') ? [nk_sim_io] : []
link_target = meson.is_cross_build() ? libfs : libavrix

# ───────────────────── 4 · Core unit-test list  ───────────────────────
tests = [
  ['test_fixed_point',      ['test_fixed_point.c']],
  ['fs_test',               ['fs_test.c',               'sim.c']],
  ['flock_stress',          ['flock_stress.c',          'sim.c']],
  ['spin_test',             ['spin_test.c',             'sim.c']],
  ['unified_spinlock_test', ['unified_spinlock_test.c', 'sim.c']],
  ['romfs_test',            ['romfs_test.c',            'sim.c']],
  ['fs_roundtrip',          ['fs_roundtrip.c',          'sim.c']],
  ['door_test',             ['door_test.c']],
]

# Native-only test
if not meson.is_cross_build()
  tests += [
    ['kalloc_test',
     ['kalloc_test.c', meson.project_source_root() / 'src/kalloc.c']]
  ]
endif

# ───────────────────── 5 · Build + register tests  ────────────────────
foreach t : tests
  exe = executable(
    t[0],
    t[1],
    include_directories : inc_list,
    link_with           : [link_target] + extra_libs,
    c_args              : test_cflags,
    link_args           : test_ldflags,
    native              : true
  )
  test(t[0], exe)
endforeach

# ───────────────────── 6 · simavr smoke tests (cross) ─────────────────
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
        s[0],
        s[1],
        include_directories : inc_list,
        c_args              : test_cflags
      )

      test(
        s[0],
        simavr,
        args        : ['-m', 'atmega328p', sim_exe],
        is_parallel : false,
        timeout     : 30
      )
    endforeach
  endif
endif
