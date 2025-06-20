# ─── tests/meson.build ────────────────────────────────────────────────
# Unit-test targets are only meaningful for a *native* build because
# Meson cannot execute AVR binaries.  When cross-compiling (is_cross),
# we still compile the test sources to catch syntax errors, but we do
# **not** register them with `test()`.

fsmod     = import('fs')
avr_root  = '/usr/lib/avr/include'
inc_list  = [inc]
if fsmod.is_dir(avr_root)
  inc_list += include_directories(avr_root)
endif

common_cflags = [
  '-O2', '-Wall', '-Wextra', '-pedantic',
  '-std=c17', '-march=native'
]

# --------------------------------------------------------------------
# 1.  Fixed-point Q8.8 boundary test
# --------------------------------------------------------------------
fp_sources = ['test_fixed_point.c']

if meson.is_cross_build()
  # Cross build → compile for syntax-check only, no runtime test
  executable(
    'test_fixed_point_x',
    fp_sources,
    include_directories : inc_list,
    c_args              : common_cflags
  )
else
  test_fp_exe = executable(
    'test_fixed_point',
    fp_sources,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('q8_8_mul_boundaries', test_fp_exe)
endif

# --------------------------------------------------------------------
# 2.  Filesystem & spin-lock stress tests (native only)
# --------------------------------------------------------------------
if not meson.is_cross_build()
  fs_test = executable(
    'fs_test',
    ['fs_test.c', meson.project_source_root() / 'src/fs.c'],
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('fs_basic', fs_test)

  flock_test = executable(
    'flock_stress',
    'flock_stress.c',
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('flock_stress', flock_test)
endif
