cdrfs = custom_target(
	'cdboot_rfs',
    output : 'init.rfs',
    input : [
		'init.sys',
		driver_map.get('iso9660'),
		driver_map.get('ata'),
		driver_map.get('AT_kbrd'),
		driver_map.get('pci'),
	],
    command : rdfs_command,
	build_by_default: true
)