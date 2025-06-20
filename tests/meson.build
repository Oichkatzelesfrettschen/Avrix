fsmod = import('fs')
avr_root = '/usr/lib/avr/include'
inc_list = [inc]
if fsmod.is_dir(avr_root)
  inc_list += include_directories(avr_root)
endif
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif
common_flags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17',
                '-march=native']

fs_test = executable('fs_test',
    ['fs_test.c', meson.project_source_root() / 'src/fs.c'] + extra_src,
    include_directories: inc_list,
    c_args: common_flags,
    native: true)

test('fs_basic', fs_test)

flock_test = executable('flock_stress',
    ['flock_stress.c'] + extra_src,
    include_directories: inc_list,
    c_args: common_flags,
    native: true)

test('flock_stress', flock_test)
