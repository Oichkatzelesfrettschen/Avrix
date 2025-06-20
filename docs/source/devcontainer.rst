Using the Devcontainer
======================

The repository ships a ready-made VS Code *devcontainer* for quick
experimentation.  It provisions the cross tool-chain, Meson, QEMU and
all documentation helpers.

#. Install the `ms-vscode.remote-containers` extension.
#. Run ``Remote‑Containers: Open Folder in Container…`` and select this
   directory.
#. The container automatically configures a ``build`` directory using
   ``meson setup build --cross-file cross/atmega328p_gcc14.cross``.
#. Reconfigure with ``meson setup build --wipe --cross-file
   cross/atmega328p_gcc14.cross`` when toggling compiler options.
#. Edit sources, then invoke ``meson compile -C build`` to recompile.

The Docker image mirrors the steps in ``setup.sh`` and therefore
matches CI precisely.
