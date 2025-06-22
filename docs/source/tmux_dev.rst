.. _tmux-dev:

===============================
Tmux-based development session
===============================

``scripts/tmux-dev.sh`` arranges a four-pane layout that mirrors the
recommended workflow:

1. **Build** – runs ``meson compile -C build`` repeatedly.
2. **Serial monitor** – attaches to ``/dev/ttyACM0`` via ``screen``.
3. **Editor** – opens ``$EDITOR`` in the project root.
4. **Shell** – spare shell for Git commands or ad-hoc tools.

Run it from the repository root::

   ./scripts/tmux-dev.sh

No tmux plugins are required. Each pane starts in the project root so that
relative paths behave as expected.
