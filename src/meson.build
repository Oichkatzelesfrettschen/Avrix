sources = files('fixed_point.c', 'fs.c', 'door.c', 'nk_fs.c', 'kalloc.c')
if host_machine.cpu_family() == 'avr'
  sources += files('task.c')
endif
if host_machine.cpu_family() != 'avr'
  sources += files('avr_stub.c')
endif
libavrix = static_library('avrix', sources,
    include_directories : inc,
    install : true
)

install_headers('../include/fixed_point.h',
                '../include/fs.h',
                '../include/nk_lock.h',
                '../include/door.h',
                '../include/memguard.h',
                '../include/nk_task.h',
                '../include/nk_fs.h',
                '../include/kalloc.h',
                subdir: 'avrix')
