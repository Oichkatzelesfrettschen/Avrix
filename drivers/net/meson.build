# ─── drivers/net/meson.build ─────────────────────────────────────────
#
# Networking drivers (SLIP, IPv4)
# ──────────────────────────────────────────────────────────────────────

net_driver_sources = files(
  'slip.c',    # RFC 1055 SLIP protocol (Serial Line IP)
  'ipv4.c',    # IPv4 stack with RFC 1071 checksum
)

net_driver_headers = files(
  'slip.h',
  'ipv4.h',
)

# Export for parent build
net_drivers_dep = declare_dependency(
  sources             : net_driver_sources,
  include_directories : include_directories('.'),
)
