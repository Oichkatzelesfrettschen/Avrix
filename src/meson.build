# ─── src/meson.build ──────────────────────────────────────────────────
#
# Cross-vs-native strategy
# ------------------------
# • **AVR / cross build** → `libavrix.a`   (full kernel + ASM)
#   This archive is the firmware that ends up on the Uno.
#
# • **Native build**      → `libavrix_host.a`
#   Hosts docs / unit-tests; contains only portable helpers so we do
#   not pull <avr/io.h>.  AVR-specific sources are still *compiled* with
#   `-fsyntax-only` to detect regressions, but never archived.
#
# • `libfs.a` (RAM FS only) is *always* produced for the host so tests
#   can link even when the parent build is a cross compile.
#
# NOTE: `inc` comes from the top-level meson.build
#       (`inc = include_directories('include')`).

# ───────────────────────── 1. Source lists ────────────────────────────
kernel_src = files(
  'fixed_point.c',
  'fs.c',
  'door.c',
  'task.c',            # pure C
  'nk_fs.c',
  'kalloc.c',
  'context_switch.S'   # hand-written AVR ASM
)

portable_src = files(   # no <avr/...>
  'fixed_point.c',
  'fs.c'
)

avr_only_src = []
foreach f : kernel_src
  if not portable_src.contains(f)
    avr_only_src += f
  endif
endforeach

# ───────────────────────── 2. libavrix / libavrix_host ───────────────
if meson.is_cross_build()          # ==> target = AVR
  libavrix = static_library(
    'avrix',
    kernel_src,
    include_directories : inc,
    install : true
  )
else                                # ==> host build (CI, docs, etc.)
  libavrix = static_library(
    'avrix_host',
    portable_src,
    include_directories : inc,
    install : false
  )

  # Keep AVR-specific sources compiling (syntax-only) so they never rot
  executable(
    'syntax_check_kernel_x',
    avr_only_src,
    include_directories : inc,
    c_args : ['-Werror', '-fsyntax-only']
  )
endif

# ───────────────────────── 3. Host-only libfs  ───────────────────────
libfs = static_library(
  'fs',
  'fs.c',
  include_directories : inc,
  native  : true,        # always build for host arch
  install : false
)

# ───────────────────────── 4. Public headers  ────────────────────────
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
