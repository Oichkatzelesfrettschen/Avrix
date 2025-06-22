#!/usr/bin/env bash
# ==========================================================================
# tmux-dev.sh — 4-pane tmux session helper for Avrix
# --------------------------------------------------------------------------
#  Pane layout (2×2):
#    [1] Build           – upper left, runs the latest build command
#    [2] Serial monitor  – upper right, opens /dev/ttyACM0 via screen
#    [3] Editor          – lower left, launches \$EDITOR or vi
#    [4] Shell           – lower right, ordinary interactive shell
#
#  This works with plain tmux and requires no plugins.  Start it from
#  anywhere inside the repository:
#       ./scripts/tmux-dev.sh
# ==========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SESSION="avrix"

# Start detached session with build pane
TMUX= tmux new-session -d -s "$SESSION" -c "$ROOT" -n build \
  "bash -c 'meson compile -C build; exec bash'"

# Serial monitor pane on the right
TMUX= tmux split-window -h -t "$SESSION":0 -c "$ROOT" \
  "screen /dev/ttyACM0 115200 || bash"

# Editor pane below build
TMUX= tmux select-pane -t "$SESSION":0.0
TMUX= tmux split-window -v -t "$SESSION":0 -c "$ROOT" \
  "${EDITOR:-vi}"

# Shell pane below serial monitor
TMUX= tmux select-pane -t "$SESSION":0.1
TMUX= tmux split-window -v -t "$SESSION":0 -c "$ROOT" "bash"

TMUX= tmux select-pane -t "$SESSION":0.0
TMUX= tmux attach -t "$SESSION"
