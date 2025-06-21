# ─── tests/meson.build ──────────────────────────────────────────────
#
#  • Cross-build (→ AVR)  →  tests always compile **native:true**
#    and link only against the portable helper library **libfs**.
#
#  • Native build          →  tests link against the full host stub
#    **libavrix** so we exercise as much code as possible.
#
#  • A tiny Python linter (`check_docs.py`) always runs and fails the
#    suite if any Sphinx toctree reference is dangling.
#
#  Imported from the root meson.build
#      inc       → include_directories('include')
#      libavrix  → host stub (native only)
#      libfs     → RAM-filesystem helpers (always)
#
# --------------------------------------------------------------------

python_mod = import('python')
python     = python_mod.find_installation('python3')
fs_mod     = import('fs')

# ────────────────────── 0. Documentation linter ─────────────────────
test(
  'check-docs',
  python,
  args : [
    files('check_docs.py'),
    '--docs-dir',  meson.project_source_root() / 'docs/source',
    '--index-file', meson.project_source_root() / 'docs/source/index.rst',
    '--recursive',
  ],
  workdir : meson.project_source_root()
)

# ────────────────────── 1. Include-path negotiation ─────────────────
#
# inc_list is shared by *all* tests.
inc_list = [inc]

# A host build needs stub <avr/...> headers in compat/avr
if host_machine.cpu_family() != 'avr'
  inc_list += include_directories('../compat')
endif

# Allow user override of an installed AVR include dir
opt_avr_inc = get_option('avr_inc_dir')   # Project option in meson_options.txt
if opt_avr_inc != '' and fs_mod.is_dir(opt_avr_inc)
  inc_list += include_directories(opt_avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# ────────────────────── 2. Common compiler flags ────────────────────
common_cflags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17']
# (Some host-only tests can be built with ASan/UBSan when CC supports it.)
if not meson.is_cross_build() and cc.get_id() == 'gcc'
  common_cflags += ['-fsanitize=address,undefined']
endif

native_mode = meson.is_cross_build()      # TRUE == tests build for host

# Convenience helper ----------------------------------------------------------
build_test = (name, srcs, deps) => executable(
  name,
  srcs,
  include_directories : inc_list,
  link_with            : deps,
  c_args               : common_cflags,
  native               : true  # always compile for host CPU
)

# Extra stub for <avr/io.h> replacement on the host
extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

# Pick correct link target for each mode
link_portable = libfs
link_full     = meson.is_cross_build() ? libfs : libavrix

# ────────────────────── 3. Unit-tests definitions  ──────────────────
tests = [
  ['test_fixed_point',   ['test_fixed_point.c',
                          meson.project_source_root() / 'src/fixed_point.c']
                         + extra_src],
  ['fs_test',            ['fs_test.c', 'sim.c']   + extra_src],
  ['flock_stress',       ['flock_stress.c', 'sim.c'] + extra_src],
  ['spin_test',          ['spin_test.c', 'sim.c'] + extra_src],
  ['superlock_test',     ['superlock_test.c', 'sim.c'] + extra_src],
  ['romfs_test',         ['romfs_test.c', 'sim.c'] + extra_src],
  ['fs_roundtrip',       ['fs_roundtrip.c', 'sim.c'] + extra_src],
]

foreach t : tests
  exe = build_test(t[0], t[1], link_full)
  test(t[0], exe)
endforeach

# ────────────────────── 4. simavr smoke-test (AVR builds) ───────────
#
# Cross-compiled suite runs *one* flash image inside simavr to catch
# stray AVR-only code paths. Host builds skip this.

if target_machine.cpu_family() == 'avr'
  simavr = find_program('simavr', required : false)
  if simavr.found()
    simavr_exe = executable(
      'fs_simavr_basic',
      ['fs_simavr_basic.c'],
      include_directories : inc_list,
      link_with           : libavrix,
      c_args              : ['-Os', '-std=c11', '-Wall', '-Wextra', '-pedantic']
    )

    test(
      'fs_simavr_basic',
      simavr,
      args         : ['-m', 'atmega328p', simavr_exe],
      is_parallel  : false,        # simavr is single-threaded
      timeout      : 30
    )
  endif
endif
