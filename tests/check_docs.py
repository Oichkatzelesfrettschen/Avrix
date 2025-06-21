Below is a **drop-in replacement** for `tests/check_docs.py`.
It removes merge artefacts, deduplicates imports, unifies CLI options and adds
a few convenience features (quiet mode, multi-extension support).

```python
#!/usr/bin/env python3
"""
check_docs.py – fail the build if any Sphinx ``.. toctree::`` entry points
to a non-existent document.

Usage examples
──────────────
    # CI / Meson target (non-recursive: only index.rst)
    python3 tests/check_docs.py --docs-dir docs/source

    # Local check, follow nested toctrees, accept .rst and .md pages
    python3 tests/check_docs.py -r -e .rst -e .md -q
Exit status: 0 = OK · 1 = missing refs · 2 = fatal error.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterator, List, Set, Tuple

# ── defaults relative to the repository root ─────────────────────────────
DEFAULT_DOCS_DIR = Path(__file__).resolve().parents[1] / "docs" / "source"


# ── internal helpers ─────────────────────────────────────────────────────


def _iter_toctree_refs(rst_file: Path) -> Iterator[str]:
    """Yield every raw entry inside all ``.. toctree::`` blocks."""
    inside, base_indent = False, None
    for line in rst_file.read_text(encoding="utf-8").splitlines():
        stripped = line.lstrip()

        if stripped.startswith(".. toctree::"):
            inside, base_indent = True, None
            continue
        if not inside:
            continue

        if not stripped or stripped.startswith(":"):
            # blank / option line
            continue
        if base_indent is None:
            base_indent = len(line) - len(stripped)
        elif len(line) - len(stripped) < base_indent:
            # dedent → block ends
            inside = False
            continue

        yield stripped.split("<!--", 1)[0].strip()  # drop inline comments


def _walk_refs(
    root: Path,
    *,
    docs_dir: Path,
    recursive: bool,
    exts: Tuple[str, ...],
    seen: Set[Path] | None = None,
) -> List[str]:
    """Collect refs from *root* (and optionally its children)."""
    seen = seen or set()
    root = root.resolve()
    if root in seen:
        return []
    seen.add(root)

    refs: List[str] = []
    for ref in _iter_toctree_refs(root):
        refs.append(ref)
        if not recursive:
            continue

        # try to resolve the reference to a file we can open
        for ext in exts:
            child = docs_dir / (ref if ref.endswith(ext) else f"{ref}{ext}")
            if child.exists():
                refs.extend(
                    _walk_refs(
                        child,
                        docs_dir=docs_dir,
                        recursive=True,
                        exts=exts,
                        seen=seen,
                    )
                )
                break
    return refs


def _missing_paths(
    refs: List[str],
    *,
    docs_dir: Path,
    exts: Tuple[str, ...],
) -> List[Path]:
    """Return every referenced file that cannot be found."""
    missing: List[Path] = []
    for ref in refs:
        # reference may include an extension already
        if Path(ref).suffix:
            candidate = docs_dir / ref
            if not candidate.exists():
                missing.append(candidate)
        else:
            # try any allowed extension
            for ext in exts:
                if (docs_dir / f"{ref}{ext}").exists():
                    break
            else:
                missing.append(docs_dir / f"{ref}{exts[0]}")
    return missing


# ── CLI / entry-point ────────────────────────────────────────────────────


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--docs-dir",
        type=Path,
        default=DEFAULT_DOCS_DIR,
        help="Documentation *source* directory (default: %(default)s)",
    )
    p.add_argument(
        "--index-file",
        type=Path,
        help="Root file to scan (default: <docs-dir>/index.rst)",
    )
    p.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Recursively follow references in nested toctree blocks.",
    )
    p.add_argument(
        "-e",
        "--ext",
        action="append",
        default=[".rst"],
        metavar=".EXT",
        help="Accepted file extension (repeatable, default: .rst).",
    )
    p.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Suppress success message.",
    )
    return p


def main(argv: list[str] | None = None) -> int:
    try:
        args = _build_parser().parse_args(argv)
        docs_dir = args.docs_dir.resolve()
        index = (args.index_file or docs_dir / "index.rst").resolve()

        if not index.exists():
            print(f"[fatal] index file not found: {index}", file=sys.stderr)
            return 2

        # normalise extensions
        exts = tuple(ext if ext.startswith(".") else f".{ext}" for ext in args.ext)

        refs = _walk_refs(
            index,
            docs_dir=docs_dir,
            recursive=args.recursive,
            exts=exts,
        )
        missing = _missing_paths(refs, docs_dir=docs_dir, exts=exts)

        if missing:
            print("\nMissing documentation pages:")
            for path in missing:
                print(f"  • {path.relative_to(docs_dir)}")
            print(f"\n✖ {len(missing)} unresolved reference(s).")
            return 1

        if not args.quiet:
            print("✓ All toctree references resolve.")
        return 0

    except Exception as exc:  # noqa: BLE001
        print(f"[fatal] unexpected error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
```

### What changed?

* **No merge markers** or duplicate imports (`argparse` appeared twice).
* `--docs-dir`, `--index-file`, multi-`--ext` and `--quiet` CLI flags.
* Single recursive walker with a `seen` set → avoids infinite loops.
* Missing files grouped in a tidy list with relative paths.
* Distinct exit codes: `0=OK`, `1=missing refs`, `2=fatal/usage`.

Copy the file into `tests/check_docs.py`; the existing Meson target
(`test('check-docs', python, args : files('check_docs.py'), …)`) will just work.
