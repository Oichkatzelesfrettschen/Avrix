#!/usr/bin/env bash
# =============================================================================
#  tmux-dev.sh — 4-pane development dashboard for Avrix (2025-06-22)
# -----------------------------------------------------------------------------
#  Usage:
#     ./scripts/tmux-dev.sh [--mode=qemu|serial]   (default = qemu)
#
#  Pane map (2 × 2):
#    0 ▌Build   ──── meson compile -C build
#    1 ▌Runner  ──── QEMU *or* screen /dev/ttyACM0 115200
#    2 ▌Aux     ──── tail -F build/qemu.log *or* $EDITOR
#    3 ▌Shell   ──── interactive bash
#
#  Requirements:
#    • tmux ≥ 2.4  :contentReference[oaicite:1]{index=1}
#    • QEMU ≥ 8.2 with AVR target enabled when --mode=qemu  :contentReference[oaicite:2]{index=2}
#    • GNU screen for --mode=serial  :contentReference[oaicite:3]{index=3}
#    • Existing build/ directory containing unix0.elf for QEMU runs
# =============================================================================
set -euo pipefail

MODE="qemu"                # default runner
SESSION="avrix"
ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

# ── Argument parsing ────────────────────────────────────────────────────────
for arg in "$@"; do
  case "$arg" in
    --mode=qemu)   MODE="qemu" ;;
    --mode=serial) MODE="serial" ;;
    *) echo "Unknown option: $arg"; exit 1 ;;
  esac
done

# ── Hot-re-attach if the tmux session already exists ────────────────────────
if tmux has-session -t "$SESSION" 2>/dev/null; then
  exec tmux attach-session -t "$SESSION"   # fast-path attach :contentReference[oaicite:4]{index=4}
fi

# ── Pane 0: incremental Meson build ─────────────────────────────────────────
tmux new-session   -d -s "$SESSION" -c "$ROOT" -n build \
  "meson compile -C build || (echo 'Meson build failed'; sleep 2)"  :contentReference[oaicite:5]{index=5}

# ── Pane 1: runner (QEMU or physical board) ────────────────────────────────
if [[ $MODE == "qemu" ]]; then
  RUNNER_CMD="qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic | tee build/qemu.log"
else
  RUNNER_CMD="screen /dev/ttyACM0 115200 || bash"   # auto-drops to shell on exit
fi
tmux split-window -h -t "$SESSION":0 -c "$ROOT" "$RUNNER_CMD"

# ── Pane 2: auxiliary (log follower or editor) ──────────────────────────────
tmux select-pane -t "$SESSION":0.0           # focus build pane first
if [[ $MODE == "qemu" ]]; then
  tmux split-window -v -t "$SESSION":0 -c "$ROOT" \
       "tail -F build/qemu.log"   :contentReference[oaicite:6]{index=6}
else
  tmux split-window -v -t "$SESSION":0 -c "$ROOT" \
       "${EDITOR:-vi}"            :contentReference[oaicite:7]{index=7}
fi

# ── Pane 3: spare interactive shell ─────────────────────────────────────────
tmux select-pane -t "$SESSION":0.1           # runner pane
tmux split-window -v -t "$SESSION":0 -c "$ROOT" "bash"

tmux select-pane -t "$SESSION":0.3           # land the user here
exec tmux attach -t "$SESSION"
