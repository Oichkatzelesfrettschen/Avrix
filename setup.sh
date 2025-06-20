#!/usr/bin/env bash
#────────────────────────────────────────────────────────────────────────────
# setup.sh — µ-UNIX / AVR tool-chain + QEMU bootstrapper
# Tested on Ubuntu 22.04 & 24.04 (2025-06 snapshots)
#
# Usage:  sudo ./setup.sh [--legacy|--modern|--deb-sid]
#   --modern   (default)  → gcc-avr 14.x via Debian pin
#   --deb-sid             → same as --modern (explicit)
#   --legacy              → gcc-avr 7.3 from Ubuntu universe
#
# The script:
#   1. Installs/updates compiler, QEMU, build helpers
#   2. Builds QEMU AVR fork if distro package lacks avr-softmmu
#   3. Configures Meson cross file, compiles demo firmware
#   4. Boots it in QEMU (arduino-uno machine) for a smoke-test
#────────────────────────────────────────────────────────────────────────────
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR

[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

mode=${1:---modern}
case "$mode" in --modern|--deb-sid|--legacy|"") ;; *)
  echo "Usage: sudo $0 [--modern|--deb-sid|--legacy]" >&2; exit 1;;
esac
[[ -z "$mode" || "$mode" == "--modern" ]] && mode="--modern"
echo "[info] Selected mode: $mode"

#──────────────── helpers ──────────────────────────────────────────────────
pkg_installed() { dpkg -s "$1" &>/dev/null; }
have_repo()     { grep -RH "$1" /etc/apt/*sources* 2>/dev/null || true; }

add_ppa_once()  { have_repo "$1" || add-apt-repository -y -n "ppa:$1"; }

#──────────────── 0. core utilities ───────────────────────────────────────
apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                      ca-certificates gnupg git

#──────────────── 1. compiler repos ───────────────────────────────────────
TOOLCHAIN_PKG=gcc-avr
case "$mode" in
  --legacy)
    add-apt-repository -y universe ;;
  --modern|--deb-sid)
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
    ;;
esac

apt-get -qq update

#──────────────── 2. install packages ─────────────────────────────────────
BASE_PKGS=(
  "$TOOLCHAIN_PKG" avr-libc binutils-avr avrdude gdb-avr
  qemu-system-misc meson ninja-build doxygen python3-sphinx python3-pip
  ccache cloc cscope exuberant-ctags cppcheck graphviz nodejs npm
)
for p in "${BASE_PKGS[@]}"; do pkg_installed "$p" || apt-get -yqq install "$p"; done

pip3 install --break-system-packages -q --upgrade breathe exhale sphinx-rtd-theme
npm install -g --silent prettier

#──────────────── 3. QEMU sanity check ────────────────────────────────────
if ! qemu-system-avr -version &>/dev/null; then
  echo "[warn] qemu-system-avr not present — building avr-softmmu target …"
  apt-get -yqq install pkg-config libglib2.0-dev autoconf automake \
                       libpixman-1-dev libgtk-3-dev
  git clone --depth 1 https://github.com/seharris/qemu-avr /opt/qemu-avr
  ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu --disable-werror \
        >/dev/null && make -s -j"$(nproc)" && make install )
fi

#──────────────── 4. print tool versions ──────────────────────────────────
echo "---------------------------------------------------------------------"
echo "avr-gcc   : $(avr-gcc -dumpversion)"
echo "avr-libc  : $(dpkg-query -W -f='${Version}\n' avr-libc)"
echo "qemu-avr  : $(qemu-system-avr --version | head -1)"
echo "---------------------------------------------------------------------"

#──────────────── 5. CFLAG helper (for copy-paste) ────────────────────────
MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -fno-unwind-tables -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
echo "[info] Suggested flags:"
echo "  export CFLAGS=\"$CFLAGS\""
echo "  export LDFLAGS=\"$LDFLAGS\""

#──────────────── 6. bootstrap project build & QEMU smoke test ───────────
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

echo "───────────────────────────────────────────────────────────────────────"
echo "µ-UNIX AVR tool-chain + QEMU environment is ready.  Happy hacking!"
echo "───────────────────────────────────────────────────────────────────────"
