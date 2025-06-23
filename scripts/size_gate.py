#!/usr/bin/env python3
"""Flash size gate helper.

This script measures the flash footprint of one or more ELF binaries using
``avr-size``.  It fails when any binary exceeds the configured ``limit`` and
records the measured size in the file pointed to by ``output``.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def flash_usage(elf: Path) -> int:
    """Return flash footprint of *elf* in bytes using ``avr-size``."""
    out = subprocess.check_output(['avr-size', str(elf)], text=True)
    lines = [l for l in out.splitlines() if l.strip()]
    if len(lines) < 2:
        raise RuntimeError(f'Unexpected avr-size output for {elf}')
    parts = lines[-1].split()
    text, data = int(parts[0]), int(parts[1])
    return text + data


def main(argv: list[str]) -> int:
    """Entry point.

    Parameters
    ----------
    argv:
        Command-line arguments excluding the program name.  The expected
        form is ``ELF... LIMIT OUTPUT`` where ``ELF`` denotes one or more
        binaries to check, ``LIMIT`` is the flash ceiling in bytes and
        ``OUTPUT`` is the file to write the final size to.
    """

    if len(argv) < 3:
        print('usage: size_gate.py <elf> [<elf> ...] <limit> <output>',
              file=sys.stderr)
        return 1

    limit = int(argv[-2])
    out_path = Path(argv[-1])
    elfs = [Path(e) for e in argv[:-2]]

    status = 0
    max_size = 0
    for elf in elfs:
        size = flash_usage(elf)
        max_size = max(max_size, size)
        if size > limit:
            print(f'[size-gate] {elf} : {size} bytes > {limit}',
                  file=sys.stderr)
            status = 1
        else:
            print(f'[size-gate] {elf} : {size} bytes OK')

    out_path.write_text(f"{max_size}\n")
    return status


if __name__ == '__main__':  # pragma: no cover - CLI entry
    sys.exit(main(sys.argv[1:]))
