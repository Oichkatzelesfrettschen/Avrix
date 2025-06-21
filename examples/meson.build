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
