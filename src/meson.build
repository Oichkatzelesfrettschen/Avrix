sources = files('fixed_point.c', 'fs.c', 'spinlock.c', 'task.c')
libavrix = static_library('avrix', sources,
    include_directories : inc,
    install : true
)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/spinlock.h',
                '../include/task.h',
                subdir: 'avrix')
