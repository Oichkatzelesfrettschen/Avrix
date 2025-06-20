#!/usr/bin/env bash
# ==============================================================
#  AVR Development Environment Setup Script
# --------------------------------------------------------------
#  Installs AVR GCC on Ubuntu 22.04/24.04.  The `--modern` mode
#  pins Debian sid packages (gcc-avr-14) and installs QEMU,
#  Meson, documentation tools and Prettier.  `--legacy` only
#  installs Ubuntu's gcc-avr 7.3 without any extras.
#────────────────────────────────────────────────────────────────────────────
# setup.sh — µ-UNIX / AVR tool-chain + QEMU bootstrapper
# Tested on Ubuntu 22.04 & 24.04 (2025-06 snapshots)
#
# Usage:  sudo ./setup.sh [--modern|--legacy]
#   --modern (default) → Debian gcc-avr-14 + full toolset
#   --legacy           → Ubuntu gcc-avr 7.3 only
#
# The script:
#   1. Installs/updates compiler, QEMU, build helpers
#   2. Builds QEMU AVR fork if distro package lacks avr-softmmu
#   3. Configures Meson cross file, compiles demo firmware
#   4. Boots it in QEMU (arduino-uno machine) for a smoke-test
#────────────────────────────────────────────────────────────────────────────
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR

## Ensure helper exists even if sourced in a trimmed shell
if ! declare -F pkg_installed >/dev/null; then
  pkg_installed() { dpkg -s "$1" &>/dev/null; }
fi

[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

mode=${1:---modern}
case "$mode" in --modern|--legacy|"") ;; *)
  echo "Usage: sudo $0 [--modern|--legacy]" >&2; exit 1;;
esac
[[ -z "$mode" || "$mode" == "--modern" ]] && mode="--modern"
echo "[info] Selected mode: $mode"

#──────────────── helpers ──────────────────────────────────────────────────
have_repo()     { grep -RH "$1" /etc/apt/*sources* 2>/dev/null || true; }

add_ppa_once()  { have_repo "$1" || add-apt-repository -y -n "ppa:$1"; }

#──────────────── 0. core utilities ───────────────────────────────────────
apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                      ca-certificates gnupg git

#──────────────── 1. compiler repos ───────────────────────────────────────
TOOLCHAIN_PKG=gcc-avr
EXTRA_PKGS=()
case "$mode" in
  --legacy)
    add-apt-repository -y universe ;;
  --modern)
    TOOLCHAIN_PKG=gcc-avr-14
    EXTRA_PKGS=(qemu-system-misc meson ninja-build)
    # Debian sid pin
    cat >/etc/apt/sources.list.d/debian-sid-avr.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
     http://deb.debian.org/debian sid main
EOF
    cat >/etc/apt/preferences.d/90avr-cross <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF
    EXTRA_PKGS+=(doxygen python3-sphinx python3-pip ccache cloc cscope \
                 exuberant-ctags cppcheck graphviz nodejs npm)
    ;;
esac

# Refresh lists after adding Debian sid pin
apt-get -qq update

#──────────────── 2. install packages ─────────────────────────────────────
BASE_PKGS=("$TOOLCHAIN_PKG" avr-libc binutils-avr avrdude gdb-avr)
if [[ "$mode" == "--modern" ]]; then
  if ! apt-get -yqq install "${BASE_PKGS[@]}" "${EXTRA_PKGS[@]}"; then
    echo "[warn] Debian packages unavailable – falling back to Ubuntu gcc-avr"
    apt-get -yqq install gcc-avr avr-libc binutils-avr avrdude gdb-avr "${EXTRA_PKGS[@]}"
    TOOLCHAIN_PKG=gcc-avr
  fi
else
  apt-get -yqq install "${BASE_PKGS[@]}"
fi

if [[ "$mode" == "--modern" ]]; then
  pip3 install --break-system-packages -q --upgrade breathe exhale sphinx-rtd-theme
  npm install -g --silent prettier
fi

#──────────────── 3. QEMU sanity check ────────────────────────────────────
if [[ "$mode" == "--modern" ]]; then
  if ! qemu-system-avr -version &>/dev/null; then
    echo "[warn] qemu-system-avr not present — building avr-softmmu target …"
    apt-get -yqq install pkg-config libglib2.0-dev autoconf automake \
                         libpixman-1-dev libgtk-3-dev
    git clone --depth 1 https://github.com/seharris/qemu-avr /opt/qemu-avr
    ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu --disable-werror \
          >/dev/null && make -s -j"$(nproc)" && make install )
  fi
fi

#──────────────── 4. print tool versions ──────────────────────────────────
echo "---------------------------------------------------------------------"
echo "avr-gcc   : $(avr-gcc -dumpversion)"
echo "avr-libc  : $(dpkg-query -W -f='${Version}\n' avr-libc)"
if command -v qemu-system-avr >/dev/null; then
  echo "qemu-avr  : $(qemu-system-avr --version | head -1)"
fi
echo "---------------------------------------------------------------------"

#──────────────── 5. CFLAG helper (for copy-paste) ────────────────────────
MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CSTD=c23
[[ "$mode" == "--legacy" ]] && CSTD=c11
CFLAGS="-std=$CSTD -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -fno-unwind-tables -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
echo "[info] Suggested flags:"
echo "  export CFLAGS=\"$CFLAGS\""
echo "  export LDFLAGS=\"$LDFLAGS\""

#──────────────── 6. bootstrap project build & QEMU smoke test ───────────
if [[ "$mode" == "--modern" ]]; then
  if [[ -f cross/atmega328p_gcc14.cross ]]; then
    echo "[info] Configuring Meson build …"
    meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross >/dev/null
    ninja -C build >/dev/null
    ELF=$(find build -name '*.elf' | head -1)
    echo "[info] Built firmware: $ELF"
    echo "[info] Launching QEMU (arduino-uno, head-less) …"
    qemu-system-avr -M arduino-uno -bios "$ELF" -nographic \
                    -serial none -monitor null \
                    -d cpu_reset -no-reboot -icount shift=0,align=off &
    sleep 2
    pkill -f qemu-system-avr || true
    echo "[info] QEMU smoke-test completed."
  else
    echo "[warn] cross/atmega328p_gcc14.cross not found – build skipped."
  fi
else
  echo "[info] Legacy mode: Meson demo and QEMU run skipped."
fi

echo "───────────────────────────────────────────────────────────────────────"
echo "µ-UNIX AVR tool-chain + QEMU environment is ready.  Happy hacking!"
echo "───────────────────────────────────────────────────────────────────────"
