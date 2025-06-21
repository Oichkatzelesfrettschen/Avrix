#!/usr/bin/env python3
"""Verify that all Sphinx toctree references resolve to existing files."""

from __future__ import annotations

from pathlib import Path
import sys
import argparse

# Paths ---------------------------------------------------------------------
# Documentation source directory and main index file.
DEFAULT_DOCS_DIR = Path(__file__).resolve().parents[1] / "docs" / "source"
DEFAULT_INDEX_FILE = DEFAULT_DOCS_DIR / "index.rst"


def parse_references(index_path: Path, *, recursive: bool = False, _seen: set[Path] | None = None) -> list[str]:
    """Return references from all ``.. toctree::`` blocks within *index_path*.

    The parser intentionally ignores directive options such as ``:maxdepth:``
    and supports multiple toctree blocks.  When *recursive* is ``True`` the
    parser will follow references to other ``.rst`` files and accumulate their
    references as well.
    """
    refs: list[str] = []
    inside = False
    indent: int | None = None

    if _seen is None:
        _seen = {index_path.resolve()}

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
        refs.append(stripped)
        if recursive:
            target = index_path.parent / (
                stripped if stripped.endswith(".rst") else f"{stripped}.rst"
            )
            real = target.resolve()
            if target.exists() and real not in _seen:
                _seen.add(real)
                refs.extend(parse_references(target, recursive=True, _seen=_seen))

    return refs


def check_references(refs: list[str], *, docs_dir: Path) -> list[Path]:
    """Return any document paths that do not exist within *docs_dir*."""
    missing: list[Path] = []
    for ref in refs:
        target = docs_dir / (ref if ref.endswith(".rst") else f"{ref}.rst")
        if not target.exists():
            missing.append(target)
    return missing


def main(argv: list[str] | None = None) -> int:
    """Entry point for command-line usage."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--docs-dir",
        type=Path,
        default=DEFAULT_DOCS_DIR,
        help="Directory containing documentation sources",
    )
    parser.add_argument(
        "--index-file",
        type=Path,
        help="Index file to parse (defaults to <docs-dir>/index.rst)",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Recursively scan referenced files",
    )

    args = parser.parse_args(argv)

    docs_dir = args.docs_dir
    index_file = args.index_file or docs_dir / "index.rst"

    refs = parse_references(index_file, recursive=args.recursive)
    if missing := check_references(refs, docs_dir=docs_dir):
        for path in missing:
            print(f"Missing file: {path}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
