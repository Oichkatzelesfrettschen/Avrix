# ─── tests/meson.build ──────────────────────────────────────────────
#
#  • Native build   → link tests against the full **libavrix** host stub
#                     to exercise as much code as possible.
#  • Cross-build    → compile tests for the *host* CPU (`native:true`)
#                     and link only against the portable **libfs** helpers.
#
#  • check_docs.py  → always runs; fails if any Sphinx toctree link is dead.
#  • Optional host extras:
#        -Dsan=yes   → GCC / Clang Address- + Undefined-sanitiser
#        -Dcov=yes   → produce llvm-cov *.profraw artefacts
#
#  Variables imported from the root meson.build
#        inc        → include_directories('include')
#        libavrix   → host-side stub (native builds only)
#        libfs      → portable RAM-FS (always)
# -------------------------------------------------------------------

python_mod = import('python')
python     = python_mod.find_installation('python3')
fs_mod     = import('fs')

# ───────────── 0 · Sphinx toctree linter (always) ───────────────────
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

# ───────────── 1 · Include-path negotiation (host vs AVR) ───────────
inc_list = [inc]

if host_machine.cpu_family() != 'avr'
  inc_list += include_directories('../compat')   # stub <avr/...>
endif

opt_avr_inc = get_option('avr_inc_dir')          # meson_options.txt
if opt_avr_inc != '' and fs_mod.is_dir(opt_avr_inc)
  inc_list += include_directories(opt_avr_inc)
elif fs_mod.is_dir('/usr/lib/avr/include')
  inc_list += include_directories('/usr/lib/avr/include')
endif

# ───────────── 2 · Compiler flags & helper utility ------------------
common_cflags = ['-O2', '-Wall', '-Wextra', '-pedantic', '-std=c17']

# Sanitiser / coverage toggles (host only)
if not meson.is_cross_build()
  if get_option('san') == 'yes' and cc.get_id() in ['gcc','clang']
    common_cflags += ['-fsanitize=address,undefined']
    add_project_arguments('-fsanitize=address,undefined', language : 'c')
    test_suites = {'env' : {'ASAN_OPTIONS' : 'detect_leaks=1'}}
  endif
  if get_option('cov') == 'yes' and cc.has_argument('-fprofile-instr-generate')
    common_cflags += ['-fprofile-instr-generate', '-fcoverage-mapping']
    add_project_arguments('-fprofile-instr-generate', language : 'c')
  endif
endif

# Convenience: always compile test objects for the host CPU
build_test = (name : str, sources, link_target) =>
  executable(
    name,
    sources,
    include_directories : inc_list,
    link_with            : link_target,
    c_args               : common_cflags,
    native               : true
  )

extra_src = []
if host_machine.cpu_family() != 'avr'
  extra_src += meson.project_source_root() / 'src/avr_stub.c'
endif

link_full = meson.is_cross_build() ? libfs : libavrix   # choose library

# ───────────── 3 · Core unit-tests list --------------------------------
tests = [
  ['test_fixed_point',   ['test_fixed_point.c']                        ],
  ['fs_test',            ['fs_test.c',        'sim.c']                ],
  ['flock_stress',       ['flock_stress.c',   'sim.c']                ],
  ['spin_test',          ['spin_test.c',      'sim.c']                ],
  ['superlock_test',     ['superlock_test.c', 'sim.c']                ],
  ['romfs_test',         ['romfs_test.c',     'sim.c']                ],
  ['fs_roundtrip',       ['fs_roundtrip.c',   'sim.c']                ],
]

foreach t : tests
  exe = build_test(t.get(0), t.get(1) + extra_src, link_full)
  test(t.get(0), exe)
endforeach

# ───────────── 4 · simavr smoke-tests (only when cross-building) -----
if meson.is_cross_build()
  simavr = find_program('simavr', required : false)
  if simavr.found()

    sim_tests = [
      ['fs_simavr_basic',   ['fs_simavr_basic.c', meson.project_source_root() / 'src/fs.c']],
      ['spinlock_isr',      ['spinlock_isr.c',    meson.project_source_root() / 'src/nk_lock.c']],
    ]

    foreach st : sim_tests
      sim_exe = executable(
        st.get(0),
        st.get(1) + extra_src,
        include_directories : inc_list,
        c_args              : ['-Os', '-std=c11', '-Wall', '-Wextra', '-pedantic']
      )

      test(
        st.get(0),
        simavr,
        args        : ['-m', 'atmega328p', sim_exe],
        is_parallel : false,
        timeout     : 30
      )
    endforeach
  endif
endif
