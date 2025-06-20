sources = files('fixed_point.c', 'fs.c', 'door.c', 'task.c', 'nk_fs.c', 'kalloc.c')
if meson.is_cross_build()
  libavrix = static_library('avrix', sources,
      include_directories : inc,
      install : true)
endif

# Lightweight filesystem library built from fs.c only. This is compiled for the
# native machine so that unit tests can link against it even when cross
# compiling the firmware.
libfs = static_library('fs', 'fs.c',
    include_directories: inc,
    native: meson.is_cross_build(),
    install: false)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/nk_lock.h',
                '../include/door.h',
                '../include/task.h',
                '../include/nk_fs.h',
                '../include/kalloc.h',
                subdir: 'avrix')
