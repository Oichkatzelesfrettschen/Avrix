#!/bin/sh
# ==============================================================
#  AVR Development Environment Setup Script
# --------------------------------------------------------------
#  Installs the AVR toolchain on Ubuntu 24.04 "Noble". By default
#  it pulls Atmel-branded packages from Peter de Bruijn's PPA but
#  it can fall back to the official universe repository.
#
#  Usage: sudo ./setup.sh [--official]
#
#  Pass --official to skip the PPA and use Ubuntu's stock packages.
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

# Determine whether to use the PPA or the official repository.
if [ "${1:-}" = "--official" ]; then
    add-apt-repository -y universe
else
    add-apt-repository -y ppa:pmjdebruijn/avr
fi
apt-get update

# Install the toolchain components.
apt-get install -y gcc-avr avr-libc binutils-avr avrdude gdb-avr

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

