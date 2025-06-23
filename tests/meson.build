# ──────────── tests/meson.build ────────────────────────────────────────
#
# Native build   → links tests against the complete **libavrix** stub.
# Cross build    → compiles tests for the host CPU (`native: true`),
#                  linking only against portable **libfs** helpers.
#
# Options defined (meson_options.txt):
#   -Dsan=true      → Enable Address & UB sanitizers (host only)
#   -Dcov=true      → Enable LLVM coverage flags (host only)
#   -Davr_inc_dir=… → Path to custom AVR include directory
#
# Imported symbols from root meson.build:
#   inc       → include_directories('include')
#   libavrix  → Host stub library (native builds)
#   libfs     → Portable RAM-based filesystem helpers (always included)
# -----------------------------------------------------------------------

python = import('python').find_installation('python3')
fs_mod = import('fs')
cc     = meson.get_compiler('c')

san = get_option('san')
cov = get_option('cov')

# 0 · Documentation Check
test(
  'check-docs',
  python,
  args : [
    files('check_docs.py'),
    '--docs-dir',   meson.project_source_root() / 'docs/source',
    '--index-file', meson.project_source_root() / 'docs/source/index.rst',
    '--recursive',
  ],
  workdir : meson.project_source_root()
)

# 1 · Include Paths Configuration
inc_list = [inc]

if host_machine.cpu_family() != 'avr'
  inc_list += include_directories(meson.project_source_root() / 'compat')
endif

avr_inc = get_option('avr_inc_dir')
if avr_inc != '' and fs_mod.is_dir(avr_inc)
  inc_list += include_directories(avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# 2 · Compiler and Sanitizer Flags Setup
common_cflags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c23']
common_link_args = []

if not meson.is_cross_build()
  if san and cc.get_id() in ['gcc', 'clang']
    common_cflags    += ['-fsanitize=address,undefined']
    common_link_args += ['-fsanitize=address,undefined']
  endif

  if cov and cc.has_argument('-fcoverage-mapping')
    common_cflags    += ['-fprofile-instr-generate', '-fcoverage-mapping']
    common_link_args += ['-fprofile-instr-generate', '-fcoverage-mapping']
  endif
endif

# 3 · Host-only Stub Source Inclusion
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

link_target = meson.is_cross_build() ? libfs : libavrix

# 4 · Core Unit Tests Definition
tests = [
  ['test_fixed_point',      ['test_fixed_point.c']],
  ['fs_test',               ['fs_test.c', 'sim.c']],
  ['flock_stress',          ['flock_stress.c', 'sim.c']],
  ['spin_test',             ['spin_test.c', 'sim.c']],
  ['unified_spinlock_test', ['unified_spinlock_test.c', 'sim.c']],
  ['superlock_test',        ['superlock_test.c', 'sim.c']],
  ['romfs_test',            ['romfs_test.c', 'sim.c']],
  ['fs_roundtrip',          ['fs_roundtrip.c', 'sim.c']],
  ['door_test',             ['door_test.c']],
]

# Include kalloc_test exclusively in native builds
if not meson.is_cross_build()
  tests += [
    ['kalloc_test',
     ['kalloc_test.c', meson.project_source_root() / 'src/kalloc.c']]
  ]
endif

# 5 · Building and Registering Tests
foreach t : tests
  srcs = t[1]

  if not srcs.contains('sim.c')
    srcs += extra_src
  endif

  exe = executable(
    t[0],
    srcs,
    include_directories : inc_list,
    link_with           : link_target,
    c_args              : common_cflags,
    link_args           : common_link_args,
    native              : true
  )

  test(t[0], exe)
endforeach

# 6 · simavr-based Smoke Tests (Cross-builds Only)
if target_machine.cpu_family() == 'avr'
  simavr = find_program('simavr', required : false)

  if simavr.found()
    sim_suites = [
      ['fs_simavr_basic', ['fs_simavr_basic.c', meson.project_source_root() / 'src/fs.c']],
      ['spinlock_isr',    ['spinlock_isr.c', meson.project_source_root() / 'src/nk_lock.c']],
    ]

    foreach s : sim_suites
      sim_exe = executable(
        s[0],
        s[1] + extra_src,
        include_directories : inc_list,
        c_args              : ['-Os', '-std=c11', '-Wall', '-Wextra', '-pedantic']
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
