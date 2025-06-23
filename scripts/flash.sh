#!/usr/bin/env bash
# ===========================================================================
#  flash.sh — Arduino Uno R3 flashing helper (ELF/HEX aware)
# ---------------------------------------------------------------------------
#  Usage:
#     scripts/flash.sh [firmware.{elf,hex}] [port]
#
#  Environment overrides:
#     MCU=atmega328p       (→ avrdude -p)
#     PROGRAMMER=arduino   (→ avrdude -c)
#     PORT=/dev/ttyACM1    (→ avrdude -P, auto-detect if empty)
#
#  Behaviour:
#     • Accepts ELF or Intel-HEX; picks avrdude format code automatically.:contentReference[oaicite:0]{index=0}
#     • If PORT unset, first /dev/ttyACM* is chosen.:contentReference[oaicite:1]{index=1}
#     • After programming, dumps lfuse/hfuse/efuse/lock bytes.:contentReference[oaicite:2]{index=2}
# ===========================================================================
set -euo pipefail

MCU=${MCU:-atmega328p}        # alias m328p also accepted by avrdude:contentReference[oaicite:3]{index=3}
PROGRAMMER=${PROGRAMMER:-arduino}   # stk500v1 bootloader preset:contentReference[oaicite:4]{index=4}

IMG=${1:-build/unix0.elf}     # firmware path (default ELF from Meson)
PORT=${2:-${PORT:-${FLASH_PORT:-}}}

# ── auto-detect USB CDC ACM port ────────────────────────────────────────────
if [[ -z "$PORT" ]]; then
  PORT=$(ls /dev/ttyACM* 2>/dev/null | head -n1 || true)
fi
[[ -z "$PORT" ]] && { echo "[error] no /dev/ttyACM* device found"; exit 1; }

# ── sanity checks ──────────────────────────────────────────────────────────
[[ -f "$IMG" ]] || { echo "[error] file not found: $IMG"; exit 1; }

# ── pick avrdude format code ───────────────────────────────────────────────
case "${IMG##*.}" in
  elf|ELF) FMT=e ;;          # little-endian ELF → “e” :contentReference[oaicite:5]{index=5}
  hex|HEX) FMT=i ;;          # Intel-HEX → “i”
  *)       FMT=a ;;          # let avrdude auto-detect → “a” :contentReference[oaicite:6]{index=6}
esac

echo "[info] Flashing $IMG → $PORT (MCU=$MCU, PROG=$PROGRAMMER)"
avrdude -c "$PROGRAMMER" -p "$MCU" -P "$PORT" -D \
        -U "flash:w:$IMG:$FMT"                     # -D: skip chip-erase :contentReference[oaicite:7]{index=7}

echo
echo "[info] Fuse / lock bytes after flash:"
avrdude -c "$PROGRAMMER" -p "$MCU" -P "$PORT" \
        -U lfuse:r:-:i -U hfuse:r:-:i -U efuse:r:-:i -U lock:r:-:i |
        awk '/lfuse|hfuse|efuse|lock/'
