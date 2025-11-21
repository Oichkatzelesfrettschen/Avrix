# ─── drivers/net/meson.build ─────────────────────────────────────────
#
# Networking drivers (SLIP, IPv4)
# ──────────────────────────────────────────────────────────────────────

net_driver_sources = []

if get_option('net_slip_enabled')
  net_driver_sources += files('slip.c')
endif

if get_option('net_ipv4_enabled')
  net_driver_sources += files('ipv4.c')
endif

# Export for parent build
net_drivers_dep = declare_dependency(
  sources             : net_driver_sources,
  include_directories : include_directories('.'),
)
