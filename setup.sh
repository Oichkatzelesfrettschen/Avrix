#!/usr/bin/env bash
# ════════════════════════════════════════════════════════════════════
#  setup.sh — µ-UNIX / AVR tool-chain + QEMU bootstrapper
#
#  Tested on Ubuntu 22.04 & 24.04 (June-2025 snapshots)
#
#  Modes        · --modern   (default)  → gcc-avr 14.x via Debian-sid pin
#               · --deb-sid               alias for --modern
#               · --legacy               → gcc-avr 7.3 from Ubuntu
#
#  What it does · installs/updates compiler, QEMU, Meson, doc tools…
#               · builds QEMU avr-softmmu if distro package lacks it
#               · configures Meson, compiles demo firmware
#               · boots it in QEMU for a smoke-test
# ════════════════════════════════════════════════════════════════════
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR
[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }

export DEBIAN_FRONTEND=noninteractive
mode="${1:---modern}"
case "$mode" in --modern|--deb-sid|"") mode="--modern" ;;
           --legacy)                   ;;
           *) echo "Usage: sudo $0 [--modern|--legacy]" >&2; exit 1 ;;
esac
echo "[info] Selected mode: $mode"

# ───────────────────────── Helpers ──────────────────────────
pkg_installed() { dpkg -s "$1" &>/dev/null; }
have_repo()     { grep -RH "$1" /etc/apt/*sources* 2>/dev/null || true; }

apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                      ca-certificates gnupg git

# ───────────────────────── Repositories ─────────────────────
if [[ $mode == "--modern" ]]; then
  # Debian sid (only gcc-avr≥14 in 2025)
  if ! have_repo "deb.debian.org/debian sid"; then
    cat >/etc/apt/sources.list.d/debian-sid-avr.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
     http://deb.debian.org/debian sid main
EOF
    cat >/etc/apt/preferences.d/90-avr <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF
  fi
else
  add-apt-repository -y universe
fi
apt-get -qq update

TOOLCHAIN_PKG=gcc-avr          # candidate will be 14.x or 7.3

# ───────────────────────── Packages ─────────────────────────
BASE_PKGS=(
  "$TOOLCHAIN_PKG" avr-libc binutils-avr avrdude gdb-avr
  qemu-system-misc simavr
  meson ninja-build doxygen python3-sphinx python3-pip python3-venv
  cloc cscope exuberant-ctags cppcheck graphviz nodejs npm
)
for p in "${BASE_PKGS[@]}"; do
  pkg_installed "$p" || apt-get -yqq install "$p"
done

# ── Docs venv ────────────────────────────────────────────────
DOC_VENV=/opt/avrix-docs
[[ -d $DOC_VENV ]] || python3 -m venv "$DOC_VENV"
"$DOC_VENV/bin/pip" install -q --upgrade pip breathe exhale sphinx-rtd-theme
npm install -g --silent prettier

# ── QEMU check / build fallback ─────────────────────────────
if ! command -v qemu-system-avr &>/dev/null; then
  echo "[warn] qemu-system-avr missing – building avr-softmmu …"
  apt-get -yqq install pkg-config libglib2.0-dev autoconf automake \
                       libpixman-1-dev libgtk-3-dev
  git clone --depth 1 https://github.com/seharris/qemu-avr /opt/qemu-avr
  ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu --disable-werror \
        >/dev/null && make -s -j"$(nproc)" && make install )
fi

# ───────────────────────── Versions ─────────────────────────
echo "───────── Tool versions ─────────"
echo "avr-gcc  : $(avr-gcc -dumpversion)"
echo "avr-libc : $(dpkg-query -W -f='${Version}\n' avr-libc)"
echo "qemu-avr : $(qemu-system-avr --version | head -1)"
echo "──────────────────────────────────"

# ─────────────────── Suggested compiler flags ───────────────
MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CFLAGS="-std=c23 -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
echo "[info] Suggested flags:"
echo "  export CFLAGS=\"$CFLAGS\""
echo "  export LDFLAGS=\"$LDFLAGS\""

# ─────────────────── Demo build + smoke-boot ────────────────
if [[ -f cross/atmega328p_gcc14.cross ]]; then
  echo "[info] Building demo firmware with Meson …"
  meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross >/dev/null
  ninja -C build >/dev/null
  ELF=$(find build -name '*.elf' | head -1)
  echo "[info] Firmware: $ELF"
  echo "[info] QEMU smoke-boot (2 s) …"
  qemu-system-avr -M arduino-uno -bios "$ELF" -nographic -serial null \
                  -monitor none -no-reboot -icount shift=0,align=off &
  QPID=$!; sleep 2; kill "$QPID" 2>/dev/null || true
else
  echo "[warn] cross/atmega328p_gcc14.cross missing – build skipped."
fi

echo "════════ setup.sh complete — AVR tool-chain & QEMU ready ════════"
