# ─── config/meson.build ──────────────────────────────────────────────
#
#  Configuration Generator ("The Modulator")
# ──────────────────────────────────────────────────────────────────────
#  Reads meson_options.txt and generates include/avrix-config.h
#  This ensures C code has access to build-time configuration.
# ──────────────────────────────────────────────────────────────────────

conf_data = configuration_data()

# ── Kernel ──
sched_type = get_option('kernel_sched_type')
conf_data.set('CONFIG_KERNEL_SCHED_TYPE_' + sched_type.to_upper(), 1)
conf_data.set('CONFIG_KERNEL_TASK_MAX', get_option('kernel_task_max'))
conf_data.set('CONFIG_KERNEL_STACK_SIZE', get_option('kernel_stack_size'))
conf_data.set10('CONFIG_KERNEL_PANIC_ON_FAULT', get_option('kernel_panic_on_fault'))

# ── Memory ──
conf_data.set('CONFIG_MM_HEAP_SIZE', get_option('mm_heap_size'))
conf_data.set10('CONFIG_MM_KALLOC_GUARDS', get_option('mm_kalloc_guards'))

# ── IPC & Sync ──
conf_data.set10('CONFIG_IPC_DOOR_ENABLED', get_option('ipc_door_enabled'))
conf_data.set10('CONFIG_SYNC_MUTEX_ENABLED', get_option('sync_mutex_enabled'))
conf_data.set10('CONFIG_SYNC_SPINLOCK_ENABLED', get_option('sync_spinlock_enabled'))

# ── Filesystem ──
fs_enabled = get_option('fs_enabled')
conf_data.set10('CONFIG_FS_ENABLED', fs_enabled)
if fs_enabled
  conf_data.set('CONFIG_FS_MAX_FILES', get_option('fs_max_files'))
  conf_data.set('CONFIG_FS_MAX_MOUNTS', get_option('fs_max_mounts'))
  conf_data.set10('CONFIG_FS_ROMFS_ENABLED', get_option('fs_romfs_enabled'))
  conf_data.set10('CONFIG_FS_EEPFS_ENABLED', get_option('fs_eepfs_enabled'))
  conf_data.set10('CONFIG_FS_EEPFS_WEAR_LEVELING', get_option('fs_eepfs_wear_leveling'))
else
  # Zero out if disabled to be safe
  conf_data.set('CONFIG_FS_MAX_FILES', 0)
  conf_data.set('CONFIG_FS_MAX_MOUNTS', 0)
  conf_data.set('CONFIG_FS_ROMFS_ENABLED', 0)
  conf_data.set('CONFIG_FS_EEPFS_ENABLED', 0)
  conf_data.set('CONFIG_FS_EEPFS_WEAR_LEVELING', 0)
endif

# ── Network ──
net_enabled = get_option('net_enabled')
conf_data.set10('CONFIG_NET_ENABLED', net_enabled)
if net_enabled
  conf_data.set10('CONFIG_NET_IPV4_ENABLED', get_option('net_ipv4_enabled'))
  conf_data.set10('CONFIG_NET_IPV4_CHECKSUM', get_option('net_ipv4_checksum'))
  conf_data.set10('CONFIG_NET_SLIP_ENABLED', get_option('net_slip_enabled'))
  conf_data.set10('CONFIG_NET_UDP_ENABLED', get_option('net_udp_enabled'))
  conf_data.set10('CONFIG_NET_TCP_ENABLED', get_option('net_tcp_enabled'))
else
  conf_data.set('CONFIG_NET_IPV4_ENABLED', 0)
  conf_data.set('CONFIG_NET_IPV4_CHECKSUM', 0)
  conf_data.set('CONFIG_NET_SLIP_ENABLED', 0)
  conf_data.set('CONFIG_NET_UDP_ENABLED', 0)
  conf_data.set('CONFIG_NET_TCP_ENABLED', 0)
endif

# ── Drivers/Debug ──
conf_data.set10('CONFIG_TTY_ENABLED', get_option('tty_enabled'))
conf_data.set('CONFIG_TTY_BUFFERS', get_option('tty_buffers'))
conf_data.set10('CONFIG_DEBUG_GDB', get_option('debug_gdb'))

# Generate the header
avrix_config_h = configure_file(
  output : 'avrix-config.h',
  configuration : conf_data,
  install : false
)

# Export dependency so other modules can include the build dir
config_dep = declare_dependency(
  sources : avrix_config_h,
  include_directories : include_directories('.')
)
