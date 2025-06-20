# ─── tests/meson.build ──────────────────────────────────────────────────
# Everything under this directory is **host-side** test code.  When the
# parent Meson build is a cross-compile (target = AVR) we still *compile*
# the sources to catch regressions, but we do **not** execute them,
# because Meson cannot run AVR binaries natively.
#
# In a native build (CI x86-64, developer laptop, etc.) the tests are
# registered with `test()` and executed.
#
# Additional Python-based sanity checks (like broken doc links) always run
# because they target the build host, not the AVR.

# ─────────────────────────── 0. Doc-link lint  ─────────────────────────
python = import('python').find_installation('python3')

test(
  'check-docs',
  python,
  args   : files('check_docs.py'),
  workdir: meson.project_source_root()
)

# ─────────────────────────── 1. Common settings  ───────────────────────
fsmod    = import('fs')
avr_root = '/usr/lib/avr/include'          # present on most Debian/Ubuntu boxes

inc_list = [inc]                           # `inc` comes from top-level meson
if fsmod.is_dir(avr_root)
  inc_list += include_directories(avr_root)
endif

common_cflags = [
  '-O2', '-Wall', '-Wextra', '-pedantic',
  '-std=c17',            # host binaries only – no C23 libs assumed
  '-march=native'
]

# ─────────────────────────── 2. Fixed-point Q8.8 test  ─────────────────
fp_src = ['test_fixed_point.c']

if meson.is_cross_build()
  executable(              # compile-only artefact (syntax / ABI check)
    'test_fixed_point_x',
    fp_src,
    include_directories : inc_list,
    c_args              : common_cflags
  )
else
  fp_exe = executable(
    'test_fixed_point',
    fp_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('q8_8_mul_boundaries', fp_exe)
endif

# ─────────────────────────── 3. Filesystem & lock stress  ──────────────
if not meson.is_cross_build()
  fs_exe = executable(
    'fs_test',
    ['fs_test.c', meson.project_source_root() / 'src/fs.c'],
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('fs_basic', fs_exe)

  flock_exe = executable(
    'flock_stress',
    'flock_stress.c',
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('flock_stress', flock_exe)
endif
