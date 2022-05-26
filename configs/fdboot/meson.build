fdrfs = custom_target(
	'fdboot_rfs',
    output : 'init.rfs',
    input : [
		'init.sys',
		driver_map.get('fat'),
		driver_map.get('floppy'),
		driver_map.get('AT_kbrd'),
		driver_map.get('isa_dma')	
	],
    command : rdfs_command,
	build_by_default: true
)