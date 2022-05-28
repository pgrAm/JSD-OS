fdrfs = custom_target(
	'fdboot_rfs',
    output : 'init.rfs',
    input : [
		'init.sys',
		driver_map.get('fat'),
		driver_map.get('floppy'),
		driver_map.get('at_kbrd'),
		driver_map.get('isa_dma'),	
		init_loader,
	],
    command : rdfs_command,
	build_by_default: true
)