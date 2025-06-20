#!/usr/bin/env bash
#───────────────────────────────────────────────────────────────────────
#  setup.sh -- µ-UNIX AVR build-chain bootstrapper
#  Supports: Ubuntu 22.04/24.04  (tested in GitHub Actions and bare metal)
#
#  Usage: sudo ./setup.sh [--modern|--legacy]
#         --modern (default) installs gcc-avr-14 from
#                 ppa:team-gcc-arm-embedded/avr
#         --legacy installs the repo version (gcc-avr 7.3)
#
#  Idempotent: already-installed packages are skipped.
#───────────────────────────────────────────────────────────────────────
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR

[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }

export DEBIAN_FRONTEND=noninteractive

#──── parse CLI arg ────────────────────────────────────────────────────
MODE="modern"
[[ ${1:-} == "--legacy" ]] && MODE="legacy"

#──── prerequisite utils for add-apt-repository ────────────────────────
apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                       ca-certificates gnupg >/dev/null

#──── enable correct repository once ───────────────────────────────────
case $MODE in
  modern)
    if ! grep -q "team-gcc-arm-embedded" /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
        add-apt-repository -y ppa:team-gcc-arm-embedded/avr
    fi
    TOOLCHAIN_PKG='gcc-avr-14'
    ;;
  legacy)
    add-apt-repository -y universe          # universe provides gcc-avr
    TOOLCHAIN_PKG='gcc-avr'
    ;;
esac

apt-get -qq update

#──── package bundle (tool-chain + doc + dev aids) ─────────────────────
PKGS=(
  "$TOOLCHAIN_PKG" avr-libc binutils-avr
  avrdude gdb-avr simavr
  avr-g++   # C++ sketches / tests
  meson ninja-build doxygen
  python3-sphinx python3-pip
  ccache cloc cscope exuberant-ctags cppcheck
)

for p in "${PKGS[@]}"; do
  dpkg -s "$p" &>/dev/null || apt-get -yqq install "$p"
done

#──── python docs extensions ───────────────────────────────────────────
pip3 install --break-system-packages --quiet --upgrade breathe exhale

#──── show versions for audit log ──────────────────────────────────────
echo "Tool-chain versions:"
avr-gcc --version | head -n1
dpkg-query -W -f 'avr-libc %v\n' avr-libc

#──── suggest canonical flag bundle (first-principles tuned) ───────────
MCU=${MCU:-atmega328p}
F_CPU=${F_CPU:-16000000UL}

CFLAGS="-std=c23 -mmcu=${MCU} -DF_CPU=${F_CPU} -Oz -flto -mrelax \
-ffunction-sections -fdata-sections -fno-unwind-tables -mcall-prologues"
LDFLAGS="-mmcu=${MCU} -Wl,--gc-sections -flto"

cat <<EOF

────────────────────────────────────────────────────────────────────────
μ-UNIX AVR environment ready ✓
Add the following to your Meson cross file or Makefile:

  CFLAGS  = ${CFLAGS}
  LDFLAGS = ${LDFLAGS}

Happy hacking!
────────────────────────────────────────────────────────────────────────
EOF
