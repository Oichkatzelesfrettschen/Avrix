# ─── tests/meson.build ──────────────────────────────────────────────────
# Host–only unit-test harness.
#
# • When the *parent* build is a **cross compile** (target = AVR) we
#   still want to *run* the unit-tests on the **build host** (x86-64
#   CI runner).  Therefore every executable in that mode is created
#   with `native: true` and links against the host-built `libfs`
#   (portable) rather than the AVR library.
#
# • When the parent build is **native**, tests link against the full
#   `libavrix` stub so we exercise more code.
#
# • A Python doc-reference linter always runs — independent of cross/
#   native — because it has no AVR dependencies.

# ───────────────────────── 0. Doc-reference linter ─────────────────────
python = import('python').find_installation('python3')
test('check-docs',
     python,
     args    : files('check_docs.py'),
     workdir : meson.project_source_root())

# ───────────────────────── 1. Include-path logic  ──────────────────────
fsmod      = import('fs')
inc_list   = [inc]                             # `inc` from root meson
avr_inc_opt = get_option('avr_inc_dir')

if avr_inc_opt != '' and fsmod.is_dir(avr_inc_opt)
  inc_list += include_directories(avr_inc_opt)
else
  fallback_avr_root = '/usr/lib/avr/include'
  if fsmod.is_dir(fallback_avr_root)
    inc_list += include_directories(fallback_avr_root)
  endif
endif

common_cflags = [
  '-O2', '-Wall', '-Wextra', '-pedantic',
  '-std=c17',              # host compiler; no need for C23
  '-march=native'
]

# convenience: true when the *tests themselves* must be host binaries
native_mode = meson.is_cross_build()

# ───────────────────────── 2. Fixed-point boundary test ───────────────
fp_src = ['test_fixed_point.c']

if native_mode
  # compile & run on host, link with portable fs helper
  fp_exe = executable(
    'test_fixed_point',
    fp_src,
    link_with           : libfs,
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
  test('q8_8_mul_boundaries', fp_exe)
else
  # native overall build: link against avrix stub
  fp_exe = executable(
    'test_fixed_point',
    fp_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  test('q8_8_mul_boundaries', fp_exe)
endif

# ───────────────────────── 3. Filesystem & lock stress ───────────────
fs_test_src   = ['fs_test.c', 'sim.c']
flock_src     = ['flock_stress.c', 'sim.c']

if native_mode
  fs_exe = executable(
    'fs_test',
    fs_test_src,
    include_directories : inc_list,
    link_with           : libfs,
    c_args              : common_cflags,
    native              : true
  )
  test('fs_basic', fs_exe)

  flock_exe = executable(
    'flock_stress',
    flock_src,
    include_directories : inc_list,
    link_with           : libfs,
    c_args              : common_cflags,
    native              : true
  )
  test('flock_stress', flock_exe)
else
  # native build: link against full stub
  fs_exe = executable(
    'fs_test',
    ['fs_test.c', meson.project_source_root() / 'src/fs.c'],
    include_directories : inc_list,
    link_with           : libavrix,
    c_args              : common_cflags
  )
  test('fs_basic', fs_exe)

  flock_exe = executable(
    'flock_stress',
    'flock_stress.c',
    include_directories : inc_list,
    link_with           : libavrix,
    c_args              : common_cflags
  )
  test('flock_stress', flock_exe)
endif
