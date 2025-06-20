sources = files('fixed_point.c', 'fs.c', 'door.c', 'task.c')
libavrix = static_library('avrix', sources,
    include_directories : inc,
    install : true
)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/nk_lock.h',
                '../include/door.h',
                '../include/memguard.h',
                '../include/task.h',
                subdir: 'avrix')
