# ─── tests/meson.build ────────────────────────────────────────────────
#
#  Native build   → link tests against the full **libavrix** host stub.
#  Cross build    → build objects for the *host* CPU (`native:true`)
#                   and link only against the portable **libfs** helpers.
#
#  Extra toggles (see meson_options.txt):
#     -Dsan=yes        Address- & UB-San for host tests
#     -Dcov=yes        LLVM coverage flags
#     -Davr_inc_dir=…  custom <avr/...> include search path
#
#  Symbols from the root meson.build:
#     inc       → include_directories('include')
#     libavrix  → host stub (native only)
#     libfs     → portable RAM-filesystem (always)
# ---------------------------------------------------------------------

python     = import('python').find_installation('python3')
fs_mod     = import('fs')

# ───────── 0 · Sphinx :toctree: link checker ──────────────────────────
test('check-docs',
     python,
     args : [
       files('check_docs.py'),
       '--docs-dir',   meson.project_source_root() / 'docs/source',
       '--index-file', meson.project_source_root() / 'docs/source/index.rst',
       '--recursive',
     ],
     workdir : meson.project_source_root())

# ───────── 1 · Include-path negotiation (host vs. AVR) ────────────────
inc_list = [inc]

if host_machine.cpu_family() != 'avr'
  inc_list += include_directories('../compat')   # stubbed <avr/...>
endif

opt_avr_inc = get_option('avr_inc_dir')
if opt_avr_inc != '' and fs_mod.is_dir(opt_avr_inc)
  inc_list += include_directories(opt_avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# ───────── 2 · Compiler flags, sanitiser, coverage  -------------------
common_cflags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17']

if not meson.is_cross_build()
  if get_option('san') == 'yes' and cc.get_id() in ['gcc', 'clang']
    san_flags       = ['-fsanitize=address,undefined']
    common_cflags  += san_flags
    add_project_arguments(san_flags, language : 'c')
  endif
  if get_option('cov') == 'yes' and cc.has_argument('-fprofile-instr-generate')
    cov_flags       = ['-fprofile-instr-generate', '-fcoverage-mapping']
    common_cflags  += cov_flags
    add_project_arguments(cov_flags, language : 'c')
  endif
endif

# Helper: always build the test binary for the host CPU ----------------
# Build each test with the same flags on the host.


# Extra stub source for host-only builds
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

link_target = meson.is_cross_build() ? libfs : libavrix

# ───────── 3 · Core unit-tests ---------------------------------------
tests = [
  ['test_fixed_point', ['test_fixed_point.c']],
  ['fs_test',          ['fs_test.c',        'sim.c']],
  ['flock_stress',     ['flock_stress.c',   'sim.c']],
  ['spin_test',            ['spin_test.c',            'sim.c']],
  ['unified_spinlock_test',['unified_spinlock_test.c','sim.c']],
  ['romfs_test',       ['romfs_test.c',     'sim.c']],
  ['fs_roundtrip',     ['fs_roundtrip.c',   'sim.c']],
]

foreach t : tests
  exe = executable(
    t.get(0),
    t.get(1) + extra_src,
    include_directories : inc_list,
    link_with           : link_target,
    c_args              : common_cflags,
    native              : true)
  test(t.get(0), exe)
endforeach

# ───────── 4 · simavr smoke-tests (cross builds only) -----------------
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
        c_args : ['-Os', '-std=c11', '-Wall', '-Wextra', '-pedantic']
      )

      test(
        s.get(0),
        simavr,
        args        : ['-m', 'atmega328p', sim_exe],
        is_parallel : false,
        timeout     : 30)
    endforeach
  endif
endif
