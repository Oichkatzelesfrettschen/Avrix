fsmod = import('fs')
inc_list = [inc]

# AVR libc headers may reside in different directories depending on the
# toolchain. Allow the path to be specified via the `avr_inc_dir` option and
# fall back to common system locations.
avr_inc_opt = get_option('avr_inc_dir')
if avr_inc_opt != '' and fsmod.is_dir(avr_inc_opt)
  inc_list += include_directories(avr_inc_opt)
else
  fallback_avr_root = '/usr/lib/avr/include'
  if fsmod.is_dir(fallback_avr_root)
    inc_list += include_directories(fallback_avr_root)
  endif
endif
common_flags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17',
                '-march=native']

test_native = meson.is_cross_build()

fs_test = executable('fs_test',
    ['fs_test.c', 'sim.c'],
    include_directories: inc_list,
    link_with: libfs,
    c_args: common_flags,
    native: test_native)

test('fs_basic', fs_test)

flock_test = executable('flock_stress',
    ['flock_stress.c', 'sim.c'],
    include_directories: inc_list,
    link_with: libfs,
    c_args: common_flags,
    native: test_native)

test('flock_stress', flock_test)
