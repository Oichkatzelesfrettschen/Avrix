sources = files('fixed_point.c', 'fs.c', 'door.c', 'task.c', 'nk_fs.c', 'kalloc.c')
libavrix = static_library('avrix', sources,
    include_directories : inc,
    install : true
)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/nk_lock.h',
                '../include/door.h',
                '../include/task.h',
                '../include/nk_fs.h',
                '../include/kalloc.h',
                subdir: 'avrix')
