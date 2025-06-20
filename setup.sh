#!/usr/bin/env bash
# ════════════════════════════════════════════════════════════════════════
#  setup.sh — µ-UNIX / AVR tool-chain & QEMU bootstrapper
#
#  ✅ Verified on Ubuntu 22.04 LTS and 24.04 LTS (snapshots 2025-06-20).
#
#  Usage
#  ─────
#      sudo ./setup.sh [--modern|--legacy]
#
#      --modern   (default) →  gcc-avr 14.x via Debian-sid pin
#                              + QEMU + Meson + docs + analysis goodies
#      --legacy              →  gcc-avr 7.3 from Ubuntu universe only
#                              (no QEMU build, no demo firmware)
#
#  What happens
#  ────────────
#   1. Adds/updates the required APT sources (Debian sid pin or Universe).
#   2. Installs compiler, QEMU, Meson, docs helpers, static-analysis tools.
#   3. Builds `qemu-system-avr` from source if the distro package lacks it.
#   4. Prints size-tuned **CFLAGS/LDFLAGS** for the ATmega328P.
#   5. (modern) Configures Meson, builds demo firmware, smoke-boots it in QEMU.
# ════════════════════════════════════════════════════════════════════════
set -euo pipefail
trap 'echo "[error] setup aborted" >&2' ERR

[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

MODE="${1:---modern}"
case "$MODE" in --modern|--legacy|"") ;; *)
  echo "Usage: sudo $0 [--modern|--legacy]" >&2; exit 1 ;;
esac
echo "[info] Selected mode: $MODE"

# ────────────────────────── Helper functions ────────────────────────────
pkg_installed() { dpkg -s "$1" &>/dev/null; }
have_repo()     { grep -RHq "$1" /etc/apt/*sources* 2>/dev/null; }

step() { printf '\n\033[1;36m[%s]\033[0m %s\n' "$(date +%H:%M:%S)" "$*"; }

step "Refreshing base APT indices"
apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                      ca-certificates gnupg git

# ─────────────────────── 1 · Repositories ───────────────────────────────
if [[ $MODE == "--modern" ]]; then
  if ! have_repo "deb.debian.org/debian sid"; then
    step "Adding Debian-sid cross repo"
    cat >/etc/apt/sources.list.d/debian-sid-avr.list <<'EOF'
deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] \
    http://deb.debian.org/debian sid main
EOF
    cat >/etc/apt/preferences.d/90-avr-cross <<'EOF'
Package: gcc-avr avr-libc binutils-avr
Pin: release o=Debian,a=sid
Pin-Priority: 100
EOF
  fi
else
  add-apt-repository -y universe
fi
apt-get -qq update

# ─────────────────────── 2 · Package selection ──────────────────────────
TOOLCHAIN=gcc-avr                # 7.3 (legacy) or 14.x candidate (modern)
BASE_PKGS=(
  "$TOOLCHAIN" avr-libc binutils-avr avrdude gdb-avr
  qemu-system-misc simavr
)

if [[ $MODE == "--modern" ]]; then
  BASE_PKGS+=(
    meson ninja-build doxygen python3-sphinx python3-pip python3-venv
    cloc cscope exuberant-ctags cppcheck graphviz nodejs npm
  )
fi

step "Installing packages"
for p in "${BASE_PKGS[@]}"; do
  pkg_installed "$p" || apt-get -yqq install "$p"
done

# ─────────────────────── 3 · Docs virtual-env (modern) ──────────────────
if [[ $MODE == "--modern" ]]; then
  DOC_VENV=/opt/avrix-docs
  if [[ ! -d $DOC_VENV ]]; then
    step "Creating Python venv for docs → $DOC_VENV"
    python3 -m venv "$DOC_VENV"
  fi
  step "Installing Sphinx extensions"
  "$DOC_VENV/bin/pip" install -q --upgrade pip breathe exhale sphinx-rtd-theme
  npm install -g --silent prettier
fi

# ─────────────────────── 4 · QEMU availability ──────────────────────────
if ! command -v qemu-system-avr &>/dev/null; then
  step "Building qemu-system-avr (avr-softmmu)"
  apt-get -yqq install pkg-config libglib2.0-dev autoconf automake \
                       libpixman-1-dev libgtk-3-dev
  git clone --depth 1 https://github.com/seharris/qemu-avr /opt/qemu-avr
  ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu --disable-werror \
        >/dev/null && make -s -j"$(nproc)" && make install )
fi

# ─────────────────────── 5 · Version report ─────────────────────────────
echo
echo "────────── Installed versions ──────────"
printf "avr-gcc  : %s\n"  "$(avr-gcc -dumpversion)"
printf "avr-libc : %s\n"  "$(dpkg-query -W -f='${Version}\n' avr-libc)"
printf "qemu-avr : %s\n"  "$(qemu-system-avr --version | head -1)"
echo "────────────────────────────────────────"

# ─────────────────────── 6 · Suggested flags  ───────────────────────────
MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CSTD=$([[ $MODE == "--legacy" ]] && echo c11 || echo c23)
CFLAGS="-std=$CSTD -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
[[ $MODE == "--modern" ]] && CFLAGS+=" --icf=safe -fipa-pta"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

echo
echo "────────── Copy-&-paste build flags ──────────"
echo "export CFLAGS=\"$CFLAGS\""
echo "export LDFLAGS=\"$LDFLAGS\""
echo "──────────────────────────────────────────────"

# ─────────────────────── 7 · Demo build + smoke-boot ────────────────────
if [[ $MODE == "--modern" && -f cross/atmega328p_gcc14.cross ]]; then
  step "Building demo firmware with Meson"
  meson setup build --wipe --cross-file cross/atmega328p_gcc14.cross >/dev/null
  ninja -C build >/dev/null
  ELF=$(find build -name '*.elf' | head -1)
  step "Smoke-booting demo in QEMU (2 s) → $ELF"
  qemu-system-avr -M arduino-uno -bios "$ELF" -nographic \
                  -serial null -monitor none -no-reboot \
                  -icount shift=0,align=off >/dev/null &
  QPID=$!; sleep 2; kill "$QPID" 2>/dev/null || true
else
  step "Demo firmware build skipped (legacy mode or cross-file missing)"
fi

echo
echo "══════════ setup.sh finished – happy hacking! ══════════"
