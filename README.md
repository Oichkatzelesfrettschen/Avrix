````markdown

| Gap / friction point                     | Why it matters                                   | Proposed fix (PR welcome!)                    |
|------------------------------------------|--------------------------------------------------|----------------------------------------------|
| **Real-board flash script**              | `setup.sh` stops after QEMU.  Newcomers still    | Add `./flash.sh` → detects `/dev/ttyACM*`,   |
|                                          | need the exact `avrdude` incantation.            | sets correct baud + boot-reset (1200 bps).   |
| **`tmux-dev.sh` launcher**               | The README describes the 4-pane session, but     | Ship a shell wrapper that spawns tmux panes   |
|                                          | the helper script lives only in docs.            | (`slattach`, UART console, `ping`, `tail -f`).|
| **GDB remote stub**                      | Live system debugging is currently “printf + LED”.| Compile `avr-gdbstub` (≈ 1.2 kB) behind       |
|                                          |                                                  | `-DDEBUG_GDB`, use the 16U2 for CDC bridge.   |
| **`clang-format` + pre-commit**          | Style drifts between contributors.               | Add `.clang-format` identical to LLVM-style   |
|                                          |                                                  | (except 4-space indent), hook via `pre-commit`.|
| **Static-analysis CI stage**             | We run `cppcheck` locally but not in GitHub      | New workflow: `cppcheck --std=c23 --enable=all`|
|                                          | Actions.                                         | + `clang-tidy` (host build).                  |
| **Code-coverage report**                 | Unit tests execute on host but we never publish  | `llvm-cov show` on the native test binary,    |
|                                          | the coverage artefact.                           | upload to Pages.                              |
| **Power-budget regression test**         | Extra peripherals may sneak in >30 mA spikes.    | INA219 script on a HW-in-the-loop runner;     |
|                                          |                                                  | fail PR if average current > 40 mA.           |
| **Board variant autoselect**             | Clones (e.g. CH340G, Old Bootloader @ 0x7E00)    | Detect VID/PID, choose correct reset-baud and |
|                                          | need different reset pulses.                     | boot address via `board.json`.                |
| **VS Code `.devcontainer`**              | New contributors on Win/Mac need zero-setup      | Publish `devcontainer.json` with Debian-stable|
|                                          | onboarding.                                      | + sid overlay, mounts serial device.          |
| **Driver template generator**            | Boiler-plate `nk_task`, IRQ vector, YAML docs.   | `scripts/new-driver.py --name ws2812b` emits  |
|                                          |                                                  | `.c/.h`, test stub and docs skeleton.         |
| **Doxygen → Sphinx cross-link**          | API names don’t hyperlink from the manual.       | Add `breathe_projects['avrix']` in `conf.py`, |
|                                          |                                                  | run `breathe-apigen`.                         |
| **Binary size guardrail**                | No automatic alarm when `unix0.elf` ≥ 30 kB.     | Meson `custom_target('size-gate', …)` fails   |
|                                          |                                                  | if `avr-size -A` reports flash > 30720 bytes. |
| **Optional uIP + TinyFS-J flags**        | Users must edit `meson_options.txt` by hand.     | Expose `-Duip=true -Dfs=tinyfsj` on configure. |
| **RISC-V build (host tests)**            | Cross-compile CI catches little-endian only.     | Add `gcc-riscv64-unknown-elf` matrix entry.    |
| **License blurb per file**               | Root `LICENSE` is MIT but headers still lack the | Run `addlicense` pre-commit to inject SPDX.   |
|                                          | SPDX line.                                       |                                              |

---

### 📓 Road-map doc stubs

The following stub files exist but are still mostly TODO lists:

* `docs/source/roadmap-qemu-avr.rst` — path to upstream QEMU merge  
* `docs/source/filesystem.rst` — TinyFS-J design deep-dive  
* `docs/source/hardware.rst` — annotated Uno schematic

Feel free to flesh out any chapter; CI auto-publishes to GitHub Pages.

---

### 🔗  Related Meson options not yet in README

| Option                 | Default | What it toggles                          |
|------------------------|---------|------------------------------------------|
| `-Duip=true/false`     | **false** | Embed Adam Dunkels’ µIP TCP/IP stack      |
| `-Dfs=tinylog|tinyfsj` | `tinylog` | Select wear-levelled log vs. journal FS   |
| `-Dprofile=true`       | **false** | Enable PGO (build → run → rebuild)        |
| `-Dstack-guard=true`   | **true** | Insert 0xDEADBEEF sentinels per task      |

Invoke e.g.  
```bash
meson setup build -Duip=true -Dfs=tinyfsj \
                  --cross-file cross/atmega328p_gcc14.cross
````

---

### 🛠️  Scripts directory glance

```
scripts/
├── flash.sh          # auto-detect Uno, call avrdude
├── qemu-uno.sh       # wraps long qemu-system-avr CLI
├── tmux-dev.sh       # 4-pane UART / slattach / ping / log
├── new-driver.py     # scaffolds src/… driver + test + docs
└── size-gate.py      # Meson hook, enforces 30 kB flash limit
```

`make install-tools` drops the helpers into `/usr/local/bin`.

---

### ✔️  After these land…

* **On-device debugging** becomes two-step:
  `avr-gdb build/unix0.elf --eval-command="target remote :4242"`
* CI matrix gains `flash-real-unox` job flashing a real board via GitHub-hosted
  self-runner.
* Coverage badge appears next to the CI badge at the top of this README.

Until then, the core tool-chain + QEMU flow is rock-solid — but these
quality-of-life additions will shave hours off onboarding and keep flash
sprawl in check as features pile up.

*Pull requests welcomed — keep each under 1 kB flash, please!* 🐜

```
```
