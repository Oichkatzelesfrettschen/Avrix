fs_demo = executable(
  'fs_demo',
  'fs_demo.c',
  include_directories: inc,
  link_with: libavrix,
  c_args: ['-Wall', '-Wextra'],
  install: false
)

romfs_demo = executable(
  'romfs_demo',
  'romfs_demo.c',
  include_directories: inc,
  link_with: libavrix,
  c_args: ['-Wall', '-Wextra'],
  install: false
)

slip_demo = executable(
  'slip_demo',
  'slip_demo.c',
  include_directories: inc,
  link_with: libavrix,
  c_args: ['-Wall', '-Wextra'],
  install: false
)

# Convert ELF to Intel HEX for use with simavr or avrdude
objcopy = find_program('avr-objcopy', required: false)
if objcopy.found()
  custom_target('fs_demo_hex',
    command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input: fs_demo,
    output: 'fs_demo.hex')
  custom_target('romfs_demo_hex',
    command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input: romfs_demo,
    output: 'romfs_demo.hex')
  custom_target('slip_demo_hex',
    command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    input: slip_demo,
    output: 'slip_demo.hex')
endif

opt_cflags = [
  '-Wall', '-Wextra', '-Os',
  '-ffunction-sections', '-fdata-sections',
  '-fno-stack-protector'
]

ned = executable(
  'ned',
  'ned.c',
  include_directories: inc,
  link_with: libavrix,
  c_args: opt_cflags,
  install: false
)

vini = executable(
  'vini',
  'vini.c',
  include_directories: inc,
  link_with: libavrix,
  c_args: opt_cflags,
  install: false
)

if not meson.is_cross_build()
  door_demo = executable(
    'door_demo',
    'door_demo.c',
    include_directories: inc,
    c_args: ['-Wall', '-Wextra'],
    native: true,
    install: false
  )
endif
