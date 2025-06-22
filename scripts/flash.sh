#!/usr/bin/env bash
# ======================================================================
#  flash.sh -- Uno R3 flashing helper
# ----------------------------------------------------------------------
#  Usage:
#    scripts/flash.sh firmware.hex [port]
#
#  Auto-detects /dev/ttyACM* when no port is given. After programming
#  the flash, the script dumps lfuse, hfuse, efuse and lock bits.
# ======================================================================
set -euo pipefail

HEX=${1:-}
if [[ -z "$HEX" || ! -f "$HEX" ]]; then
  echo "Usage: $0 firmware.hex [port]" >&2
  exit 1
fi

PORT=${2:-${FLASH_PORT:-}}
if [[ -z "$PORT" ]]; then
  PORT=$(find /dev -maxdepth 1 -name 'ttyACM*' 2>/dev/null | head -n1 || true)
fi

if [[ -z "$PORT" ]]; then
  echo "[error] no /dev/ttyACM* device found" >&2
  exit 1
fi

PROG=arduino
MCU=m328p

printf '[info] Flashing %s to %s\n' "$HEX" "$PORT"
avrdude -c "$PROG" -p "$MCU" -P "$PORT" -D -U "flash:w:$HEX" || exit 1

echo
printf '[info] Fuse and lock bytes:\n'
avrdude -c "$PROG" -p "$MCU" -P "$PORT" \
  -U lfuse:r:-:i -U hfuse:r:-:i -U efuse:r:-:i -U lock:r:-:i \
  | awk '/lfuse|hfuse|efuse|lock/'
