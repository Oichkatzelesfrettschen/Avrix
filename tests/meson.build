python = import('python').find_installation('python3')

check_docs = files('check_docs.py')

test('check-docs', python, args: check_docs,
     workdir: meson.project_source_root())
