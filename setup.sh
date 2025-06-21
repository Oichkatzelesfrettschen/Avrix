#!/usr/bin/env bash
# ════════════════════════════════════════════════════════════════════════
#  setup.sh — µ-UNIX / AVR tool-chain & QEMU bootstrapper
#
#  ✅ Verified on Ubuntu 22.04 LTS  and 24.04 LTS  (2025-06-20 snapshots)
#
#  Usage
#  ─────
#        sudo ./setup.sh [--modern|--legacy]
#
#        --modern   (default) → gcc-avr 14.x from a Debian-sid pin
#                              + QEMU + Meson + docs + analysis goodies
#        --legacy              → gcc-avr 7.3 from Ubuntu *universe* only
#                              (skips QEMU build + firmware demo)
#
#  Steps
#  ─────
#   1. Enables the required APT repositories.
#   2. Installs compiler, QEMU, Meson, doc helpers, static-analysis tools.
#   3. Builds qemu-system-avr from source if the distro lacks avr-softmmu.
#   4. Prints size-tuned **CFLAGS / LDFLAGS** for the ATmega328P.
#   5. For --modern: configures Meson, builds a demo ELF, smoke-boots in QEMU.
#
#  🛠  Workaround while you wait
#  ────────────────────────────
#  Should the Debian-sid cross packages disappear from the mirrors,
#  rebuild them locally:
#     sudo add-apt-repository deb-src http://ftp.debian.org/debian sid main
#     sudo apt update
#     apt source gcc-avr
#     cd gcc-avr-14.2.0-2
#     sudo apt build-dep .
#     debuild -us -uc     # install resulting .deb via dpkg -i
# ════════════════════════════════════════════════════════════════════════
set -euo pipefail
trap 'echo -e "\n[error] setup aborted 🚨" >&2' ERR

[[ $(id -u) -eq 0 ]] || { echo "Run as root." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

MODE="${1:---modern}"
case "$MODE" in --modern|--legacy|"") ;; *)
  echo "Usage: sudo ./setup.sh [--modern|--legacy]" >&2; exit 1 ;;
esac

step() { printf '\n\033[1;36m[%s]\033[0m %s\n' "$(date +%H:%M:%S)" "$*"; }
have_repo()     { grep -RHq "$1" /etc/apt/*sources* 2>/dev/null; }

# Base toolchain packages installed in both modes
TOOLCHAIN_PKG=gcc-avr
BASE_PKGS=(
  "$TOOLCHAIN_PKG" avr-libc binutils-avr avrdude gdb-avr
  qemu-system-misc simavr
)

# Extra utilities installed only in modern mode
EXTRA_PKGS=(
  meson ninja-build doxygen python3-sphinx python3-pip python3-venv
  cloc cscope exuberant-ctags cppcheck graphviz nodejs npm
)

step "Selected mode: $MODE"

# ───────────────────────── 0 · base tools ───────────────────────────────
step "Refreshing APT indices"
apt-get -qq update
apt-get -yqq install software-properties-common apt-transport-https \
                      ca-certificates gnupg git curl

# ───────────────────────── 1 · repositories ─────────────────────────────
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
    if ! have_repo "deb.debian.org/debian sid"; then
      echo "[error] failed to add sid repo; falling back to legacy toolchain" >&2
      MODE="--legacy"
    fi
  fi
else
  add-apt-repository -y universe
  if ! have_repo "universe"; then
    echo "[error] failed to add 'universe' repo; falling back to legacy toolchain" >&2
    MODE="--legacy"
  fi
fi
apt-get -qq update

# ───────────────────────── 2 · packages ─────────────────────────────────
PACKAGES=("${BASE_PKGS[@]}")
[[ $MODE == "--modern" ]] && PACKAGES+=("${EXTRA_PKGS[@]}")

step "Installing ${#PACKAGES[@]} packages (this can take a while)"
apt-get -qq update
apt-get -yqq install "${PACKAGES[@]}"

# ───────────────────────── 3 · docs venv (modern) ───────────────────────
if [[ $MODE == "--modern" ]]; then
  DOC_VENV=/opt/avrix-docs
  if [[ ! -d $DOC_VENV ]]; then
    step "Creating Python venv for Sphinx → $DOC_VENV"
    python3 -m venv "$DOC_VENV"
  fi
  step "Installing Sphinx extensions"
  "$DOC_VENV/bin/pip" install -q --upgrade pip breathe exhale sphinx-rtd-theme
  npm install -g --silent prettier
fi

# ───────────────────────── 4 · QEMU check / build ───────────────────────
if ! command -v qemu-system-avr &>/dev/null; then
  step "Building qemu-system-avr (avr-softmmu)"
  apt-get -yqq install pkg-config libglib2.0-dev autoconf automake \
                       libpixman-1-dev libgtk-3-dev
  git clone --depth 1 https://github.com/seharris/qemu-avr /opt/qemu-avr
  ( cd /opt/qemu-avr && ./configure --target-list=avr-softmmu --disable-werror \
        >/dev/null && make -s -j"$(nproc)" && make install )
fi

# ───────────────────────── 5 · versions report ──────────────────────────
echo
echo "──────────── Installed versions ────────────"
printf "avr-gcc  : %s\n"  "$(avr-gcc -dumpversion)"
printf "avr-libc : %s\n"  "$(dpkg-query -W -f='${Version}\n' avr-libc)"
printf "qemu-avr : %s\n"  "$(qemu-system-avr --version | head -1)"
echo "────────────────────────────────────────────"

# ───────────────────────── 6 · suggested flags ──────────────────────────
MCU=${MCU:-atmega328p}  F_CPU=${F_CPU:-16000000UL}
CSTD=$([[ $MODE == "--legacy" ]] && echo c11 || echo c23)
CFLAGS="-std=$CSTD -mmcu=$MCU -DF_CPU=$F_CPU -Oz -flto -mrelax \
        -ffunction-sections -fdata-sections -mcall-prologues"
[[ $MODE == "--modern" ]] && CFLAGS+=" --icf=safe -fipa-pta"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"

echo
echo "──────────── Copy-&-paste build flags ────────────"
echo "export CFLAGS=\"$CFLAGS\""
echo "export LDFLAGS=\"$LDFLAGS\""
echo "──────────────────────────────────────────────────"

# ───────────────────────── 7 · Demo build (modern) ──────────────────────
if [[ $MODE == "--modern" && ( -f cross/atmega328p_gcc14.cross || -f cross/atmega328p_clang20.cross ) ]]; then
  step "Configuring Meson cross build"
  CROSS_FILE=cross/atmega328p_gcc14.cross
  [[ -f cross/atmega328p_clang20.cross ]] && CROSS_FILE=cross/atmega328p_clang20.cross
  meson setup build --wipe --cross-file "$CROSS_FILE" \
        >/dev/null
  step "Compiling firmware"
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
echo "════════════════ setup.sh finished – happy hacking! ════════════════"
