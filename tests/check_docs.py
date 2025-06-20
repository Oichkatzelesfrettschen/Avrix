#!/usr/bin/env python3
"""Verify that all Sphinx toctree references resolve to existing files."""

from __future__ import annotations

from pathlib import Path
import sys

# Paths ---------------------------------------------------------------------
# Documentation source directory and main index file.
DOCS_DIR = Path(__file__).resolve().parents[1] / "docs" / "source"
INDEX_FILE = DOCS_DIR / "index.rst"


def parse_references(index_path: Path) -> list[str]:
    """Return references from all ``.. toctree::`` blocks within *index_path*.

    The parser intentionally ignores directive options such as ``:maxdepth:``
    and supports multiple toctree blocks.
    """
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
        refs.append(stripped)

    return refs


def check_references(refs: list[str]) -> list[Path]:
    """Return any document paths that do not exist."""
    missing: list[Path] = []
    for ref in refs:
        target = DOCS_DIR / (ref if ref.endswith(".rst") else f"{ref}.rst")
        if not target.exists():
            missing.append(target)
    return missing


def main() -> int:
    refs = parse_references(INDEX_FILE)
    if missing := check_references(refs):
        for path in missing:
            print(f"Missing file: {path}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
