#!/bin/sh
# ==============================================================
#  AVR Development Environment Setup Script
# --------------------------------------------------------------
#  Installs the AVR toolchain on Ubuntu 24.04 "Noble".
#  By default the script installs the modern toolchain from the
#  `team-gcc-arm-embedded` PPA which provides GCC 14.  Passing
#  `--legacy` installs the older gcc-avr 7.3 package from Ubuntu.
#
#  Usage: sudo ./setup.sh [--modern|--legacy]
#
#  The script verifies each package with `dpkg -s` before installation,
#  automatically enables the required PPA, and performs a single
#  `apt-get update` afterwards.
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


# Ensure prerequisite utilities are installed. The `software-properties-common`
# package provides `add-apt-repository` which allows us to enable PPAs.
missing_prereqs=""
if ! command -v add-apt-repository >/dev/null 2>&1; then
    missing_prereqs="software-properties-common apt-transport-https ca-certificates"
fi

# Determine which repository to enable for the compiler packages.
case "${1:-}" in
    --legacy)
        add-apt-repository -y universe
        best_pkg=gcc-avr
        ;;
    --modern|"")
        add-apt-repository -y ppa:team-gcc-arm-embedded/avr
        best_pkg=gcc-avr-14
        ;;
    *)
        echo "Usage: $0 [--modern|--legacy]" >&2
        exit 1
        ;;
esac

# Refresh package lists after adding the repository. This single update
# precedes all package installations.
apt-get update

# Install prerequisite utilities if they were missing.
if [ -n "$missing_prereqs" ]; then
    # shellcheck disable=SC2086  # intentional word splitting for package list
    apt-get install -y $missing_prereqs
fi

# Install AVR GCC, avr-libc, binutils, avrdude, gdb, simavr and tooling.
to_install="$best_pkg avr-libc binutils-avr avrdude gdb-avr simavr \
    meson ninja-build doxygen python3-sphinx cloc cscope exuberant-ctags cppcheck"
for pkg in $to_install; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        apt-get install -y "$pkg"
    fi
done

# Python packages for Sphinx integration
pip3 install --break-system-packages --upgrade breathe exhale

# Display compiler and library versions for verification.
avr-gcc --version | head -n 1
dpkg-query -W -f 'avr-libc ${Version}\n' avr-libc

# Suggest optimised flags tuned for the selected MCU.
MCU=${MCU:-atmega328p}
F_CPU=${F_CPU:-16000000UL}

compiler_opts="-std=c23 -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax"
compiler_opts="$compiler_opts -ffunction-sections -fdata-sections -mcall-prologues"

cat <<FLAGMSG
Recommended compiler flags:
  CFLAGS="$compiler_opts"
  LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
FLAGMSG

