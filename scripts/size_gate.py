#!/usr/bin/env python3
"""Fail if the AVR firmware exceeds the flash budget."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ELF = Path(sys.argv[1])
LIMIT = 30 * 1024  # 30 KiB flash budget

try:
    out = subprocess.check_output(['avr-size', '-A', ELF], text=True)
except FileNotFoundError:
    sys.exit('avr-size not found')

# Sum sections that occupy flash memory
flash = 0
for line in out.splitlines():
    fields = line.split()
    if len(fields) >= 2 and fields[0] in {'.text', '.rodata', '.data'}:
        flash += int(fields[1])
print(out)

if flash > LIMIT:
    print(f'Firmware size {flash} bytes exceeds 30 KiB limit', file=sys.stderr)
    sys.exit(1)
