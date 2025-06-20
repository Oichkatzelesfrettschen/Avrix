project(
  'avrix', 'c',
  version: '0.1',
  license: 'MIT',
  default_options: ['c_std=c23', 'warning_level=2']
)

inc = include_directories('include')
subdir('src')
subdir('examples')
subdir('tests')
subdir('lib')

# --------------------------------------------------------------
#  Optional documentation targets
# --------------------------------------------------------------
# These generators are only available when their respective tools
# are installed.  A convenience alias `doc` aggregates the enabled
# subtargets when at least one documentation system is detected.

doc_targets = []

## Doxygen API reference #######################################
doxygen = find_program('doxygen', required : false)
if doxygen.found()

run_target('doc-doxygen',
    command: ['sh', '-c',
      'cd ' + meson.current_source_dir() + ' && ' + doxygen.path() + ' Doxyfile']

  )
endif

## Sphinx manual ###############################################
sphinx = find_program('sphinx-build', required : false)
if sphinx.found()
  doc_targets += run_target(
    'doc-sphinx',
    command : [
      sphinx,
      meson.current_source_dir() / 'docs/source',
      meson.current_build_dir() / 'docs'
    ]
  )
endif

## Aggregator ###################################################
if doc_targets.length() > 0
  alias_target('doc', doc_targets)
endif

