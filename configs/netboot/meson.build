netrfs = custom_target(
	'netboot_rfs',
    output : 'init.rfs',
    input : [
		'init.sys',
		driver_map.get('ps2mouse'),
		driver_map.get('i8042'),
		driver_map.get('at_kbrd'),
		driver_map.get('vesa'),
		listmode,
		shell,
		graphics,
		clib,
		terminal,
		init_loader,
	],
    command : rdfs_command,
	build_by_default: true
)