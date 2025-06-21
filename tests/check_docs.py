#!/usr/bin/env python3
"""Verify that all Sphinx toctree references resolve to existing files."""

from __future__ import annotations

from pathlib import Path
import argparse
import sys

# Paths ---------------------------------------------------------------------
# Documentation source directory and main index file.
DOCS_DIR = Path(__file__).resolve().parents[1] / "docs" / "source"
INDEX_FILE = DOCS_DIR / "index.rst"


def parse_references(
    index_path: Path,
    *,
    docs_dir: Path | None = None,
    recursive: bool = False,
    _visited: set[Path] | None = None,
) -> list[str]:
    """Return references from ``.. toctree::`` blocks within *index_path*.

    When *recursive* is ``True`` referenced pages are searched recursively so
    that nested ``toctree`` blocks are also inspected.  *docs_dir* defaults to
    ``index_path.parent`` and provides the base directory for resolving
    references.

    The parser intentionally ignores directive options such as ``:maxdepth:``
    and supports multiple toctree blocks.
    """
    docs_dir = docs_dir or index_path.parent
    _visited = _visited or set()
    if index_path in _visited:
        return []
    _visited.add(index_path)

    refs: list[str] = []
    inside = False
    indent: int | None = None

    for line in index_path.read_text(encoding="utf-8").splitlines():
        stripped = line.lstrip()
        if stripped.startswith(".. toctree::"):
            inside = True
            indent = None
            continue
        if not inside:
            continue
        if not stripped:
            # Blank lines are ignored within toctree blocks.
            continue
        if stripped.startswith(":"):
            # Directive option (e.g., ``:maxdepth:``).
            continue
        if indent is None:
            indent = len(line) - len(stripped)
        elif len(line) - len(stripped) < indent:
            # Dedent marks the end of the toctree block.
            inside = False
            indent = None
            continue
        entry = stripped.split("<!--", 1)[0].rstrip()
        refs.append(entry)
        if recursive:
            child = docs_dir / (
                entry if entry.endswith(".rst") else f"{entry}.rst"
            )
            if child.exists():
                refs.extend(
                    parse_references(
                        child,
                        docs_dir=docs_dir,
                        recursive=True,
                        _visited=_visited,
                    )
                )

    return refs


def check_references(refs: list[str], *, docs_dir: Path) -> list[Path]:
    """Return any document paths that do not exist under *docs_dir*."""
    missing: list[Path] = []
    for ref in refs:
        target = docs_dir / (ref if ref.endswith(".rst") else f"{ref}.rst")
        if not target.exists():
            missing.append(target)
    return missing


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--docs-dir",
        type=Path,
        default=DOCS_DIR,
        help="Sphinx documentation source directory",
    )
    parser.add_argument(
        "--index-file",
        type=Path,
        default=INDEX_FILE,
        help="Root index.rst file",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Recursively parse referenced pages",
    )
    args = parser.parse_args()

    docs_dir = args.docs_dir.resolve()
    index_file = (
        args.index_file
        if args.index_file.is_absolute()
        else docs_dir / args.index_file
    )

    refs = parse_references(index_file, docs_dir=docs_dir, recursive=args.recursive)
    if missing := check_references(refs, docs_dir=docs_dir):
        for path in missing:
            print(f"Missing file: {path}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
