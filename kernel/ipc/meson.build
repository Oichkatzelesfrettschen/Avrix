# ─── kernel/ipc/meson.build ──────────────────────────────────────────
#
# Inter-process communication (Door RPC)
# ──────────────────────────────────────────────────────────────────────

ipc_sources = files(
  'door.c',   # Zero-copy Door RPC (Solaris-style synchronous RPC)
)

ipc_headers = files(
  'door.h',
)

# Export for parent build
ipc_dep = declare_dependency(
  sources             : ipc_sources,
  include_directories : include_directories('.'),
)
