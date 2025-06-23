#!/usr/bin/env python3
"""Fail if any ELF in the build directory exceeds 30 kB of flash."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

FLASH_LIMIT = 30 * 1024  # bytes (default)


def flash_usage(elf: Path) -> int:
    """Return flash footprint of *elf* in bytes using ``avr-size``."""
    out = subprocess.check_output(['avr-size', str(elf)], text=True)
    lines = [l for l in out.splitlines() if l.strip()]
    if len(lines) < 2:
        raise RuntimeError(f'Unexpected avr-size output for {elf}')
    parts = lines[-1].split()
    text, data = int(parts[0]), int(parts[1])
    return text + data


def main(build_dir: str, limit: int) -> int:
    build = Path(build_dir)
    elfs = sorted(build.rglob('*.elf'))
    if not elfs:
        print(f'[size-gate] no ELF binaries found in {build}', file=sys.stderr)
        return 1

    status = 0
    for elf in elfs:
        size = flash_usage(elf)
        if size > limit:
            print(f'[size-gate] {elf} : {size} bytes > {limit}', file=sys.stderr)
            status = 1
        else:
            print(f'[size-gate] {elf} : {size} bytes OK')
    return status


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Check firmware size against limit.')
    parser.add_argument('build_dir', nargs='?', default='build', help='Meson build directory')
    parser.add_argument('--limit', type=int,
                        default=int(os.environ.get('FLASH_LIMIT', FLASH_LIMIT)),
                        help='Flash size limit in bytes')
    args = parser.parse_args()
    sys.exit(main(args.build_dir, args.limit))
