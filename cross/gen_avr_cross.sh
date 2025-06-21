#!/usr/bin/env bash
# ======================================================================
# gen_avr_cross.sh -- Generate cross/avr_m328p.txt from avr_m328p.in
# ----------------------------------------------------------------------
# Usage:
#   AVR_PREFIX=/opt/avr/bin ./cross/gen_avr_cross.sh
# ----------------------------------------------------------------------
# AVR_PREFIX defaults to /usr/bin when not set. A trailing slash is added
# automatically if missing.
# ======================================================================

set -euo pipefail

prefix="${AVR_PREFIX:-/usr/bin}"
[[ ${prefix: -1} != '/' ]] && prefix="${prefix}/"

sed "s|@AVR_PREFIX@|${prefix}|g" "$(dirname "$0")/avr_m328p.in" > "$(dirname "$0")/avr_m328p.txt"

echo "Generated cross/avr_m328p.txt with prefix ${prefix}" >&2
