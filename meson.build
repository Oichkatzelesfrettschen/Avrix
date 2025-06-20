project(
  'avrix', 'c',
  version: '0.1',
  license: 'MIT',
  default_options: ['c_std=c11', 'warning_level=2']
)

inc = include_directories('include')
subdir('src')
subdir('examples')

# Documentation targets
doxygen = find_program('doxygen', required: false)
if doxygen.found()
  run_target('doc-doxygen', command: [doxygen, meson.current_source_dir() / 'Doxyfile'])
endif

sphinx = find_program('sphinx-build', required: false)
if sphinx.found()
  run_target('doc-sphinx',
    command: [sphinx, meson.current_source_dir() / 'docs/source',
             meson.current_build_dir() / 'docs']
  )
endif
