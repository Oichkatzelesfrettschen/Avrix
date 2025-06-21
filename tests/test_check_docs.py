import sys
from pathlib import Path
from textwrap import dedent

# Allow import of the sibling module `check_docs.py`
sys.path.append(str(Path(__file__).resolve().parent))
import check_docs


def test_parse_references_no_blocks(tmp_path):
    idx = tmp_path / "index.rst"
    idx.write_text("Heading\n=======\n\nJust text\n", encoding="utf-8")
    assert check_docs.parse_references(idx) == []


def test_parse_references_multiple_blocks(tmp_path):
    idx = tmp_path / "index.rst"
    idx.write_text(
        dedent(
            """
            .. toctree::
               :maxdepth: 1

               first
               second

            .. toctree::
               :caption: Extra
               third.rst
               fourth
            """
        ),
        encoding="utf-8",
    )
    assert check_docs.parse_references(idx) == [
        "first",
        "second",
        "third.rst",
        "fourth",
    ]


def test_parse_references_directive_options(tmp_path):
    idx = tmp_path / "index.rst"
    idx.write_text(
        dedent(
            """
            .. toctree::
               :maxdepth: 2
               :caption: Options

               alpha
               beta
            """
        ),
        encoding="utf-8",
    )
    refs = check_docs.parse_references(idx)
    assert refs == ["alpha", "beta"]


def test_check_references_missing_and_existing(tmp_path, monkeypatch):
    docs = tmp_path / "docs"
    docs.mkdir()
    (docs / "a.rst").write_text("", encoding="utf-8")
    (docs / "b.rst").write_text("", encoding="utf-8")
    monkeypatch.setattr(check_docs, "DOCS_DIR", docs)
    missing = check_docs.check_references(["a", "b.rst", "c"])
    assert missing == [docs / "c.rst"]

