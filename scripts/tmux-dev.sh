#!/usr/bin/env bash
# ============================================================================
#  tmux-dev.sh -- four-pane tmux session helper for Avrix development
# --------------------------------------------------------------------------
#  Pane layout:
#    0: meson compile -C build              (top left)
#    1: qemu-system-avr -M arduino-uno ...  (top right, logs to build/qemu.log)
#    2: tail -f build/qemu.log              (bottom left)
#    3: interactive shell                   (bottom right)
#
#  Start from the project root.  The build directory must already exist
#  and contain a compiled ELF (unix0.elf).  QEMU output is mirrored to
#  build/qemu.log so that pane 2 can follow it in real time.
# ============================================================================
set -euo pipefail
SESSION="avrix"

# Reattach if the session already exists
if tmux has-session -t "$SESSION" 2>/dev/null; then
  exec tmux attach-session -t "$SESSION"
fi

# Pane 0: incremental build
tmux new-session -d -s "$SESSION" 'meson compile -C build'

# Pane 1: QEMU emulator with logging
TMUX_CMD="qemu-system-avr -M arduino-uno -bios build/unix0.elf -nographic | tee build/qemu.log"
tmux split-window -h -t "$SESSION":0 "$TMUX_CMD"

# Pane 2: Log follower
tmux select-pane -t "$SESSION":0.0
tmux split-window -v -t "$SESSION":0 'tail -F build/qemu.log'

# Pane 3: Spare interactive shell
tmux select-pane -t "$SESSION":0.1
tmux split-window -v -t "$SESSION":0

# Attach user to the session in pane 3
tmux select-pane -t "$SESSION":0.3
exec tmux attach-session -t "$SESSION"
