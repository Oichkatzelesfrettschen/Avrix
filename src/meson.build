# ─── src/meson.build ──────────────────────────────────────────────────
# Strategy
# --------
# • **Cross (AVR) build**  → produce `libavrix.a` containing the *entire*
#   kernel (C + ASM).  This is what actually ships to the board.
# • **Native build**       → produce a slim `libavrix_host.a` that contains
#   only portable helpers; host-side unit-tests & docs link against it.
#   AVR-specific sources are still compiled with `-fsyntax-only` to catch
#   regressions but *not* archived.
# • `libfs.a` (RAM-filesystem only) is *always* compiled for the host so
#   tests run even when the parent build is a cross-compile.
#
# NOTE: `inc` is the `include_directories('include')` object defined in
#       the top-level meson.build.

# ─────────────────────────── 1. Source lists ──────────────────────────
kernel_src = files(
  'fixed_point.c',
  'fs.c',
  'door.c',
  'task.c',
  'nk_fs.c',
  'kalloc.c',
  'context_switch.S'     # hand-written AVR context swapper
)

portable_src = files(
  'fixed_point.c',
  'fs.c'
)

avr_only_src = kernel_src + []     # copy
avr_only_src -= portable_src       # diff: AVR-specific files only

# ─────────────────────────── 2. libavrix / libavrix_host ──────────────
if meson.is_cross_build()
  libavrix = static_library(
    'avrix',
    kernel_src,
    include_directories : inc,
    install : true
  )
else
  libavrix = static_library(
    'avrix_host',
    portable_src,
    include_directories : inc,
    install : false
  )

  # Syntax-check the AVR bits so they never rot
  executable(
    'syntax_check_kernel_x',
    avr_only_src,
    include_directories : inc,
    c_args : ['-Werror', '-fsyntax-only']
  )
endif

# ─────────────────────────── 3. libfs (host-only) ─────────────────────
libfs = static_library(
  'fs',
  'fs.c',
  include_directories : inc,
  native  : true,       # always host architecture
  install : false
)

# ─────────────────────────── 4. Public headers ────────────────────────
install_headers(
  '../include/fixed_point.h',
  '../include/fs.h',
  '../include/nk_lock.h',
  '../include/door.h',
  '../include/memguard.h',
  '../include/nk_task.h',
  '../include/nk_fs.h',
  '../include/kalloc.h',
  subdir : 'avrix'
)
