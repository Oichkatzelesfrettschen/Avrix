project('avrix', 'c', version: '0.1', license: 'MIT')

inc = include_directories('include')
subdir('src')

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
  doc_targets += run_target(
    'doc-doxygen',
    command : [doxygen, meson.current_source_dir() / 'Doxyfile']
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

