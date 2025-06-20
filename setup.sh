#!/bin/sh
# ==============================================================
#  AVR Development Environment Setup Script
# --------------------------------------------------------------
#  Installs the AVR toolchain on Ubuntu 24.04 "Noble".
#  By default the script enables the `ubuntu-toolchain-r/test` repository to
#  obtain a newer host GCC.  Both modes ultimately install the `gcc-avr` package
#  provided by Ubuntu.  `--legacy` skips the PPA entirely.
#
#  Usage: sudo ./setup.sh [--modern|--legacy]
#
#  Environment variables MCU and F_CPU may be set to customise the
#  recommended compiler flags.
# ==============================================================

set -eu

# Ensure the script is run with root privileges.
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root." >&2
    exit 1
fi

# Update package lists and install supporting utilities.
apt-get update
apt-get install -y software-properties-common apt-transport-https ca-certificates

# Determine which repository to enable for the compiler packages.
case "${1:-}" in
    --legacy)
        add-apt-repository -y universe
        best_pkg=gcc-avr
        ;;
    --modern|"")
        add-apt-repository -y ppa:ubuntu-toolchain-r/test
        best_pkg=gcc-avr
        ;;
    *)
        echo "Usage: $0 [--modern|--legacy]" >&2
        exit 1
        ;;
esac
apt-get update
# Display available AVR compiler packages for reference
echo "Available AVR packages:" >&2
apt-cache search gcc-avr >&2 || true

# Install AVR GCC, avr-libc, binutils, avrdude, gdb, simavr and tooling.
apt-get install -y "$best_pkg" avr-libc binutils-avr avrdude gdb-avr simavr \
    meson ninja-build doxygen python3-sphinx cloc cscope exuberant-ctags cppcheck

# Python packages for Sphinx integration
pip3 install --break-system-packages --upgrade breathe exhale

# Display compiler and library versions for verification.
avr-gcc --version | head -n 1
dpkg-query -W -f 'avr-libc ${Version}\n' avr-libc

# Suggest optimised flags tuned for the selected MCU.
MCU=${MCU:-atmega328p}
F_CPU=${F_CPU:-16000000UL}

cat <<FLAGMSG
Recommended compiler flags:
  CFLAGS="-mmcu=$MCU -DF_CPU=$F_CPU -Os -flto -ffunction-sections -fdata-sections"
  LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
FLAGMSG

