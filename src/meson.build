if meson.is_cross_build()
  sources = files('fixed_point.c', 'fs.c', 'door.c', 'task.c')
else
  sources = files('fixed_point.c', 'fs.c')
endif

libavrix = static_library('avrix', sources,
    include_directories : inc,
    install : true
)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/nk_lock.h',
                '../include/door.h',
                '../include/task.h',
                subdir: 'avrix')
