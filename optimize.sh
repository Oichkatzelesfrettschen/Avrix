#!/usr/bin/env bash
#────────────────────────────────────────────────────────────────────────────
#  optimize.sh — run clang-tidy performance checks on the sources
#
#  Usage: ./optimize.sh [extra clang-tidy options]
#
#  Runs clang-tidy on every C file in src/ with the performance-* checks
#  enabled.  The AVR MCU can be overridden with MCU=<chip>.
#────────────────────────────────────────────────────────────────────────────
set -euo pipefail
MCU=${MCU:-atmega328p}
AVR_INC=$(realpath $(avr-gcc -print-file-name=include)/../../../../avr/include)
[[ -e include/nk_task.h ]] || { ln -s task.h include/nk_task.h; trap 'rm -f include/nk_task.h' EXIT; }
for f in src/*.c; do
  echo "[info] clang-tidy $f"
  clang-tidy "$f" --quiet \
    -checks='performance-*,-clang-analyzer-security.insecureAPI.*' -- \
    -target avr -mmcu="$MCU" -std=c23 -DF_CPU=16000000UL \
    -Iinclude -I"$AVR_INC" "$@" || true
done
