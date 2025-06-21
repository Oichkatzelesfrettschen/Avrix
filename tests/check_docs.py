#!/usr/bin/env python3
"""Verify that all Sphinx toctree references resolve to existing files."""

from __future__ import annotations

from pathlib import Path
import sys
import argparse

# Paths ---------------------------------------------------------------------
# Documentation source directory and main index file.
DEFAULT_DOCS_DIR = Path(__file__).resolve().parents[1] / "docs" / "source"
# Alias for backwards-compatibility with older tests
DOCS_DIR = DEFAULT_DOCS_DIR
DEFAULT_INDEX_FILE = DEFAULT_DOCS_DIR / "index.rst"


def parse_references(
    index_path: Path,
    *,
    recursive: bool = False,
    allowed_exts: tuple[str, ...] = (".rst",),
    _seen: set[Path] | None = None,
) -> list[str]:
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
            # Pick the first existing target using the allowed extensions.
            target: Path | None = None
            for ext in allowed_exts:
                cand = index_path.parent / (
                    stripped if stripped.endswith(ext) else f"{stripped}{ext}"
                )
                if cand.exists():
                    target = cand
                    break
            if target is None:
                # Fall back to the first extension regardless of existence.
                ext = allowed_exts[0]
                target = index_path.parent / (
                    stripped if stripped.endswith(ext) else f"{stripped}{ext}"
                )
            real = target.resolve()
            if target.exists() and real not in _seen:
                _seen.add(real)
                refs.extend(
                    parse_references(
                        target,
                        recursive=True,
                        allowed_exts=allowed_exts,
                        _seen=_seen,
                    )
                )

    return refs


def check_references(
    refs: list[str],
    *,
    docs_dir: Path | None = None,
    allowed_exts: tuple[str, ...] = (".rst",),
) -> list[Path]:
    """Return any document paths that do not exist within *docs_dir*."""
    docs_dir = DOCS_DIR if docs_dir is None else docs_dir
    missing: list[Path] = []
    for ref in refs:
        if any(ref.endswith(ext) for ext in allowed_exts):
            candidate = docs_dir / ref
            if not candidate.exists():
                missing.append(candidate)
            continue
        # Try each allowed extension and stop at the first existing one.
        found = False
        for ext in allowed_exts:
            candidate = docs_dir / f"{ref}{ext}"
            if candidate.exists():
                found = True
                break
        if not found:
            # None of the candidates exist; report the first for consistency.
            missing.append(docs_dir / f"{ref}{allowed_exts[0]}")
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
    parser.add_argument(
        "--ext",
        "--extensions",
        dest="ext",
        nargs="+",
        default=[".rst"],
        help="Allowed file extensions for document references",
    )

    args = parser.parse_args(argv)

    docs_dir = args.docs_dir
    index_file = args.index_file or docs_dir / "index.rst"
    allowed_exts = tuple(args.ext)

    if not index_file.exists():
        print(f"Index file not found: {index_file}", file=sys.stderr)
        return 2

    refs = parse_references(
        index_file,
        recursive=args.recursive,
        allowed_exts=allowed_exts,
    )
    if missing := check_references(
        refs, docs_dir=docs_dir, allowed_exts=allowed_exts
    ):
        for path in missing:
            print(f"Missing file: {path}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
