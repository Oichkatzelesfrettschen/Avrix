import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "check_docs", Path(__file__).resolve().parent / "check_docs.py"
)
check_docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_docs)


def test_no_toctree_blocks(tmp_path):
    index = tmp_path / "index.rst"
    index.write_text("Just text\nAnother line\n", encoding="utf-8")
    assert check_docs.parse_references(index) == []


def test_multiple_toctree_blocks(tmp_path):
    index = tmp_path / "index.rst"
    index.write_text(
        """
Intro

.. toctree::
   first
   second

.. toctree::
   third
""",
        encoding="utf-8",
    )
    assert check_docs.parse_references(index) == ["first", "second", "third"]


def test_directive_options_ignored(tmp_path):
    index = tmp_path / "index.rst"
    index.write_text(
        """
.. toctree::
   :maxdepth: 2
   :caption: Example

   page1
   page2
""",
        encoding="utf-8",
    )
    assert check_docs.parse_references(index) == ["page1", "page2"]


def test_references_with_and_without_rst_extension(tmp_path):
    docs_dir = tmp_path
    (docs_dir / "page1.rst").write_text("", encoding="utf-8")
    (docs_dir / "page2.rst").write_text("", encoding="utf-8")
    index = docs_dir / "index.rst"
    index.write_text(
        """
.. toctree::
   page1
   page2.rst
""",
        encoding="utf-8",
    )
    refs = check_docs.parse_references(index)
    assert refs == ["page1", "page2.rst"]
    assert check_docs.check_references(refs, docs_dir=docs_dir) == []


def test_missing_vs_existing_files(tmp_path):
    docs_dir = tmp_path
    (docs_dir / "existing.rst").write_text("", encoding="utf-8")
    index = docs_dir / "index.rst"
    index.write_text(
        """
.. toctree::
   existing
   missing
""",
        encoding="utf-8",
    )
    refs = check_docs.parse_references(index)
    missing = check_docs.check_references(refs, docs_dir=docs_dir)
    assert missing == [docs_dir / "missing.rst"]


def test_recursive_parsing(tmp_path):
    docs_dir = tmp_path
    sub = docs_dir / "sub.rst"
    sub.write_text(
        """
.. toctree::
   child
""",
        encoding="utf-8",
    )
    (docs_dir / "child.rst").write_text("", encoding="utf-8")
    index = docs_dir / "index.rst"
    index.write_text(
        """
.. toctree::
   sub
""",
        encoding="utf-8",
    )
    refs = check_docs.parse_references(index, recursive=True)
    assert refs == ["sub", "child"]


def test_allowed_extensions(tmp_path):
    docs_dir = tmp_path
    (docs_dir / "page.md").write_text("", encoding="utf-8")
    index = docs_dir / "index.rst"
    index.write_text(
        """
.. toctree::
   page
""",
        encoding="utf-8",
    )
    refs = check_docs.parse_references(index, allowed_exts=(".rst", ".md"))
    assert refs == ["page"]
    missing = check_docs.check_references(
        refs, docs_dir=docs_dir, allowed_exts=(".rst", ".md")
    )
    assert missing == []
