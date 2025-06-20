project = 'Avrix'
extensions = [
    'sphinx.ext.autodoc',
    'breathe',
    'exhale',
]

# Breathe configuration pulls Doxygen XML from the build directory.
breathe_projects = {
    'avrix': '../doxygen/xml'
}
breathe_default_project = 'avrix'

exhale_args = {
    'containmentFolder': './api',
    'rootFileName': 'library_root.rst',
    'rootFileTitle': 'API Reference',
    'doxygenStripFromPath': '../'
}
html_theme = 'alabaster'
master_doc = 'index'
