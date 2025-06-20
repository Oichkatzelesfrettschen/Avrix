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
# Prefer the Read the Docs theme when available to better match the
# online style.  Fall back to the lightweight Alabaster theme if the
# package is missing.
try:
    import sphinx_rtd_theme  # noqa: F401
    html_theme = 'sphinx_rtd_theme'
except ImportError:  # pragma: no cover - theme not installed
    html_theme = 'alabaster'
master_doc = 'index'
