project('avrix', 'c', version: '0.1', license: 'MIT')

inc = include_directories('include')
subdir('src')

# Documentation targets
doxygen = find_program('doxygen', required: false)
if doxygen.found()
  run_target('doc-doxygen',
    command: ['sh', '-c',
      'cd ' + meson.current_source_dir() + ' && ' + doxygen.path() + ' Doxyfile']
  )
endif

sphinx = find_program('sphinx-build', required: false)
if sphinx.found()
  run_target('doc-sphinx',
    command: [sphinx, meson.current_source_dir() / 'docs/source',
             meson.current_build_dir() / 'docs']
  )
endif
