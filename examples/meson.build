fs_demo = executable(
  'fs_demo',
  'fs_demo.c',
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
endif
