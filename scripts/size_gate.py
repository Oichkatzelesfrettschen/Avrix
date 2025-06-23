#!/usr/bin/env python3
"""
------------------------------------------------------------------------
size_gate.py ― Flash-footprint gate for Avrix firmware                  │
------------------------------------------------------------------------
This helper inspects one or more AVR **ELF** binaries with ``avr-size``.
If any binary exceeds the user-supplied limit, the script exits with
status 1 and prints a diagnostic.  It always writes the *largest* size
encountered to an “output-stamp” file so Meson’s timestamp tracking
remains correct.

Two invocation patterns are supported:

1. **Explicit ELFs (Meson ≥ 2025-06-22)**
     size_gate.py --limit 30720  foo.elf bar.elf stamp.file

   *All* positional arguments except the last must be ELF paths.  
   The final positional argument is the path of the stamp/output file.

2. **Build-dir sweep (legacy)**
     size_gate.py [--limit 30720] --dir build  [--output stamp.file]

   All *.elf files found recursively under ``build`` are checked.

The default limit is taken from ``--limit``, or the environment variable
``FLASH_LIMIT``, falling back to 30 kB.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

DEFAULT_LIMIT = 30 * 1024  # 30 kB


# ────────────────────────── helpers ───────────────────────────────────
def flash_usage(elf: Path) -> int:
    """Return the flash footprint (text+data) of *elf* in bytes."""
    out = subprocess.check_output(['avr-size', str(elf)], text=True)
    lines = [l for l in out.splitlines() if l.strip()]
    if len(lines) < 2:
        raise RuntimeError(f'Unexpected avr-size output for {elf}')
    text, data = (int(x) for x in lines[-1].split()[:2])
    return text + data


def gather_elves(positional: list[str], build_dir: Path | None) -> list[Path]:
    """Resolve ELF list according to CLI mode."""
    if build_dir is not None:
        return sorted(build_dir.rglob('*.elf'))
    else:
        return [Path(p) for p in positional]


# ────────────────────────── main routine ─────────────────────────────-
def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description='Reject firmware images that exceed the flash limit'
    )
    parser.add_argument(
        '--limit',
        type=int,
        default=int(os.getenv('FLASH_LIMIT', DEFAULT_LIMIT)),
        help='Flash size ceiling in bytes (default: %(default)s)',
    )
    parser.add_argument(
        '--dir',
        type=Path,
        default=None,
        help='Scan this build directory recursively for *.elf files'
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=None,
        help='Path of stamp file to write the largest size to'
    )
    parser.add_argument('positional', nargs='*',
                        help='[ELF …] stamp.file   (explicit-list mode)')

    args = parser.parse_args(argv)

    # Determine execution mode
    explicit_mode = args.dir is None
    if explicit_mode and len(args.positional) < 2:
        parser.error('explicit mode needs ≥2 positional args: ELF… OUTPUT')

    if explicit_mode:
        *elf_args, output_stamp = args.positional
        elfs = [Path(p) for p in elf_args]
        out_path = Path(output_stamp if args.output is None else args.output)
    else:
        elfs = gather_elves([], args.dir)
        if not elfs:
            print(f'[size-gate] no ELF binaries found under {args.dir}',
                  file=sys.stderr)
            return 1
        out_path = args.output or (args.dir / 'size_gate.txt')

    limit = args.limit
    status = 0
    max_size = 0

    for elf in elfs:
        size = flash_usage(elf)
        max_size = max(max_size, size)
        if size > limit:
            print(f'[size-gate] {elf} : {size} bytes > {limit}', file=sys.stderr)
            status = 1
        else:
            print(f'[size-gate] {elf} : {size} bytes OK')

    out_path.write_text(f'{max_size}\n')
    return status


if __name__ == '__main__':          # pragma: no cover
    sys.exit(main(sys.argv[1:]))
