#!/usr/bin/env bash
#======================================================================
# flash.sh -- Flash an ELF onto the Arduino Uno
# ---------------------------------------------------------------------
# Auto-detects /dev/ttyACM* when PORT is not set.  The first match is
# used as the avrdude -P argument.  MCU and PROGRAMMER may be overridden
# via environment variables.  The ELF path defaults to build/unix0.elf
# but can be supplied as the first argument.
#======================================================================
set -euo pipefail

MCU=${MCU:-atmega328p}
PROGRAMMER=${PROGRAMMER:-arduino}
PORT=${PORT:-}
ELF=${1:-build/unix0.elf}

if [[ -z "$PORT" ]]; then
    PORT=$(ls /dev/ttyACM* 2>/dev/null | head -n 1 || true)
fi

if [[ -z "$PORT" ]]; then
    echo "[error] No /dev/ttyACM* device found" >&2
    exit 1
fi

if [[ ! -f "$ELF" ]]; then
    echo "[error] ELF not found: $ELF" >&2
    exit 1
fi

echo "[info] Flashing $ELF to $PORT via $PROGRAMMER ($MCU)"
exec avrdude -c "$PROGRAMMER" -p "$MCU" -P "$PORT" -U "flash:w:$ELF:e"
