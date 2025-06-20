# ─── src/meson.build ────────────────────────────────────────────────
# * In a **cross** build (target = AVR) we build **all** kernel modules
#   into libavrix.a — this is what ends up on the microcontroller.
# * In a **native** build (host x86-64, CI, etc.) we compile only the
#   architecture-independent helpers so unit-tests / docs can link
#   without <avr/io.h>.  We still *compile* the AVR-specific sources in
#   “syntax-check mode” to keep the host compiler honest, but we do not
#   archive them into the native library.
# * A tiny `libfs.a` (RAM-filesystem only) is always produced for host
#   unit-tests, even during a cross build.
# -------------------------------------------------------------------

# Full kernel source list (AVR-only files flag themselves with <avr/...>)
kernel_src = files(
  'fixed_point.c',
  'fs.c',
  'door.c',
  'task.c',
  'nk_fs.c',
  'kalloc.c'
)

# Portable helpers that do **not** include <avr/io.h>
portable_src = files(
  'fixed_point.c',
  'fs.c'
)

# --------------------------------------------------------------------
# libavrix — what gets installed / shipped
# --------------------------------------------------------------------
if meson.is_cross_build()
  # True target library
  libavrix = static_library(
    'avrix',
    kernel_src,
    include_directories : inc,
    install : true
  )
else
  # Host-side stub (docs & tests).  Build only the portable bits.
  libavrix = static_library(
    'avrix_host',
    portable_src,
    include_directories : inc,
    install : false    # nothing to install on the host
  )

  # Also syntax-check the AVR-only files: they must still compile!
  executable(
    'syntax_check_kernel_x',
    kernel_src,
    include_directories : inc,
    c_args : ['-Werror', '-fsyntax-only']
  )
endif

# --------------------------------------------------------------------
# libfs —  RAM-filesystem only (host unit tests link this)
# --------------------------------------------------------------------
libfs = static_library(
  'fs',
  'fs.c',
  include_directories : inc,
  native : meson.is_cross_build(),   # build for host when cross-compiling
  install: false
)

# --------------------------------------------------------------------
# Public headers
# --------------------------------------------------------------------
install_headers(
  '../include/fixed_point.h',
  '../include/fs.h',
  '../include/nk_lock.h',
  '../include/door.h',
  '../include/memguard.h',
  '../include/task.h',
  '../include/nk_fs.h',
  '../include/kalloc.h',
  subdir : 'avrix'
)
