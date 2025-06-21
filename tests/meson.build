# ─── tests/meson.build ────────────────────────────────────────────────
#
# • If the *parent* build is a **cross build** (target = AVR) the test
#   executables run on the **host** CPU.  We therefore compile them with
#   `native: true` and link against **libfs** (portable helpers only).
#
# • If the parent build is **native** the tests link against the full
#   `libavrix_host` stub so that we exercise as much code as possible.
#
# • A tiny Python doc-reference linter (`check_docs.py`) always runs.
#
# Variables imported from the top-level meson.build:
#     inc        → include_directories('include')
#     libavrix   → host stub (native build only)
#     libfs      → portable RAM-filesystem (always host)
# ---------------------------------------------------------------------

# ───────────────────── 0. Doc-reference linter ───────────────────────
python = import('python').find_installation('python3')

test('check-docs',
     python,
     args    : files('check_docs.py'),
     workdir : meson.project_source_root())

# ───────────────────── 1. Include-path negotiation ───────────────────
fsmod       = import('fs')
inc_list    = [inc]                       # base include dir from root
avr_opt_dir = get_option('avr_inc_dir')   # optional – user override

if avr_opt_dir != '' and fsmod.is_dir(avr_opt_dir)
  inc_list += include_directories(avr_opt_dir)
else
  sys_avr = '/usr/lib/avr/include'
  if fsmod.is_dir(sys_avr)
    inc_list += include_directories(sys_avr)
  endif
endif

# Host builds (non-AVR) need stub replacements for <avr/io.h> symbols.
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

common_cflags = [
  '-O2', '-Wall', '-Wextra', '-pedantic',
  '-std=c17',                # host compiler, C23 not required
  '-march=native'
]

# Convenience: TRUE when tests *must* be compiled for the host
native_mode = meson.is_cross_build()

# ───────────────────── 2. Fixed-point boundary test ──────────────────
fp_src = ['test_fixed_point.c'] + extra_src

if native_mode
  fp_exe = executable(
    'test_fixed_point',
    fp_src,
    link_with           : libfs,         # portable helpers only
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
else
  fp_exe = executable(
    'test_fixed_point',
    fp_src,
    link_with           : libavrix,      # full host stub
    include_directories : inc_list,
    c_args              : common_cflags
  )
endif

test('q8_8_mul_boundaries', fp_exe)

# ───────────────────── 3. Filesystem & lock stress tests ─────────────
fs_test_src  = ['fs_test.c', 'sim.c'] + extra_src
flock_src    = ['flock_stress.c', 'sim.c'] + extra_src
spin_src     = ['spin_test.c', 'sim.c'] + extra_src
romfs_src    = ['romfs_test.c', 'sim.c'] + extra_src

if native_mode
  fs_exe = executable(
    'fs_test',
    fs_test_src,
    link_with           : libfs,
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
  flock_exe = executable(
    'flock_stress',
    flock_src,
    link_with           : libfs,
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
  spin_exe = executable(
    'spin_test',
    spin_src,
    link_with           : libfs,
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
  romfs_exe = executable(
    'romfs_test',
    romfs_src,
    link_with           : libfs,
    include_directories : inc_list,
    c_args              : common_cflags,
    native              : true
  )
else
  fs_exe = executable(
    'fs_test',
    ['fs_test.c', meson.project_source_root() / 'src/fs.c'] + extra_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  flock_exe = executable(
    'flock_stress',
    ['flock_stress.c'] + extra_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  spin_exe = executable(
    'spin_test',
    ['spin_test.c'] + extra_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
  romfs_exe = executable(
    'romfs_test',
    ['romfs_test.c'] + extra_src,
    link_with           : libavrix,
    include_directories : inc_list,
    c_args              : common_cflags
  )
endif

test('fs_basic',    fs_exe)
test('flock_stress', flock_exe)
test('spin_lock',    spin_exe)
test('romfs_basic',  romfs_exe)
