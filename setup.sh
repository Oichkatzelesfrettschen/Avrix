#!/bin/sh
# ==============================================================
#  AVR Development Environment Setup Script
# --------------------------------------------------------------
#  Installs the AVR toolchain on Ubuntu 24.04 "Noble".
#  By default the script attempts to install the newest cross compiler
#  available.  It enables the *ubuntu-toolchain-r/test* PPA and searches
#  for any gcc-<version>-avr packages.  If none are present it falls back
#  to the stock gcc-avr package.  The historic pmjdebruijn PPA can be
#  selected for older releases.
#
#  Usage: sudo ./setup.sh [--stock|--old]
#
#  --stock     Use Ubuntu's stock packages only.
#  --old       Attempt to install the deprecated pmjdebruijn PPA.
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
    --stock)
        add-apt-repository -y universe
        ;;
    --old)
        add-apt-repository -y ppa:pmjdebruijn/avr
        ;;
    "")
        add-apt-repository -y ppa:ubuntu-toolchain-r/test
        ;;
    *)
        echo "Usage: $0 [--stock|--old]" >&2
        exit 1
        ;;
esac
apt-get update

# Determine the best available compiler package.
best_pkg=$(apt-cache search gcc | grep -E '^gcc-[0-9]+-avr' | awk '{print $1}' | sort -V | tail -n 1)
if [ -z "$best_pkg" ]; then
    best_pkg=gcc-avr
fi

# Install the toolchain components.
apt-get install -y "$best_pkg" avr-libc binutils-avr avrdude gdb-avr

# Display compiler and library versions for verification.
avr-gcc --version | head -n 1
avr-libc-config --version

# Suggest optimised flags tuned for the selected MCU.
MCU=${MCU:-atmega328p}
F_CPU=${F_CPU:-16000000UL}

cat <<FLAGMSG
Recommended compiler flags:
  CFLAGS="-mmcu=$MCU -DF_CPU=$F_CPU -Os -flto -ffunction-sections -fdata-sections"
  LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
FLAGMSG

