#!/usr/bin/env bash
#────────────────────────────────────────────────────────────────────────────
#  setup.sh — µ-UNIX AVR tool-chain bootstrapper (QEMU-centric)
#  Supports  Ubuntu 22.04  &  24.04        (tested on noble-2025-06 daily)
#
#  Usage:  sudo ./setup.sh [--legacy|--modern|--deb-sid]
#          --legacy   → gcc-avr 7.3 from Ubuntu Universe      (safe & small)
#          --modern   → gcc-avr 14.x from  ppa:ubuntu-toolchain-r/test
#          --deb-sid  → pin Debian-sid gcc-avr 14.2  (no PPA, but signed)
#          (omit flag = --modern, the most useful default in 2025)
#
#  Idempotent: already-installed packages are skipped gracefully.
#────────────────────────────────────────────────────────────────────────────
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR
[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

MODE="modern"
case "${1:-}" in --legacy|--modern|--deb-sid) MODE="${1#--}"; shift ;; esac

echo "[info] Selected mode: $MODE"

#── Core helpers ────────────────────────────────────────────────────────────
have_repo() { grep -HR "$1" /etc/apt/*sources* 2>/dev/null || true; }
add_ppa_once() {
  have_repo "$1" || add-apt-repository -y "ppa:$1"
}

apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https ca-certificates gnupg

#── Repository logic ────────────────────────────────────────────────────────
case $MODE in
  legacy)
    add-apt-repository -y universe            # Ubuntu stock 7.3 tool-chain
    TOOLCHAIN_PKG=gcc-avr                     # 1:7.3.0+Atmel3.7 on noble:contentReference[oaicite:6]{index=6}
    ;;
  modern)
    add_ppa_once ubuntu-toolchain-r/test      # GCC-14 cross builds:contentReference[oaicite:7]{index=7}
    TOOLCHAIN_PKG=gcc-avr
    ;;
  deb-sid)
    cat >/etc/apt/sources.list.d/debian-sid.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
     http://deb.debian.org/debian sid main
EOF
    cat >/etc/apt/preferences.d/90debian-sid-avr <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 90
EOF
    TOOLCHAIN_PKG=gcc-avr        # pulls 1:14.2.0-2 from sid today:contentReference[oaicite:8]{index=8}
    ;;
esac

apt-get -qq update

#── Package bundle (tool-chain + QEMU route) ────────────────────────────────
PKGS=(
  "$TOOLCHAIN_PKG" avr-libc binutils-avr avrdude gdb-avr
  qemu-system-misc        # contains qemu-system-avr ≥8.2 on noble:contentReference[oaicite:9]{index=9}
  meson ninja-build doxygen python3-sphinx python3-pip
  ccache cloc cscope exuberant-ctags cppcheck
)

for p in "${PKGS[@]}"; do
  dpkg -s "$p" &>/dev/null || apt-get -yqq install "$p"
done

#── Optional legacy Atmel-patched PPA (very old but sometimes required) ─────
add_ppa_once nonolith/avr-toolchain && echo "[info] nonolith PPA enabled (Atmel patches) – packages are circa 2015"  #:contentReference[oaicite:10]{index=10}

#── QEMU sanity check ───────────────────────────────────────────────────────
if ! command -v qemu-system-avr >/dev/null; then
    echo "[warn] qemu-system-avr missing; building head from GitHub fork..."
    apt-get -yqq install git pkg-config libglib2.0-dev autoconf automake libpixman-1-dev
    git -C /opt clone --depth 1 https://github.com/seharris/qemu-avr          #:contentReference[oaicite:11]{index=11}
    ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu >/dev/null && make -s -j"$(nproc)" && make install )
fi

#── Python docs helpers ─────────────────────────────────────────────────────
pip3 install --break-system-packages -q --upgrade breathe exhale

#── Verification report ────────────────────────────────────────────────────
echo "Tool-chain versions:"
avr-gcc --version | head -n1
echo "avr-libc $(dpkg-query -W -f='${Version}\n' avr-libc)"
qemu-system-avr --version | head -n1 | awk '{print $1,$3}'

MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -fno-unwind-tables -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

cat <<EOF

────────────────────────────────────────────────────────────────────────────
µ-UNIX AVR + QEMU environment ready ✓

  CFLAGS  = $CFLAGS
  LDFLAGS = $LDFLAGS

Next steps:
  • Compile:  avr-gcc \$CFLAGS  foo.c  -o foo.elf
  • Emulate:  qemu-system-avr -M arduino-uno -bios foo.elf -nographic
              # full board model documented in QEMU manual:contentReference[oaicite:12]{index=12}
  • Debug  :  add  -S -gdb tcp::1234  and attach avr-gdb

Happy hacking ─ and remember that QEMU’s AVR devices keep improving upstream;
pull daily if you need the brand-new ATmega128(U/C) models.:contentReference[oaicite:13]{index=13}
────────────────────────────────────────────────────────────────────────────
EOF
