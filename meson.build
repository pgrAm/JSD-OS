project('jsd-os', ['c', 'cpp'])
nasm = find_program('nasm', required: true)
asm_gen = generator(nasm,
  output : '@BASENAME@.o',
  arguments : [
    '-f', 'elf',
    '@INPUT@',
    '-o', '@OUTPUT@'])

kernel_asm = asm_gen.process([
	'kernel/kernel.asm',
	'kernel/interrupt.asm',
	'kernel/paging.asm',
	'kernel/syscall.asm'
])
kernel_src = [	
	'kernel/kernel.c',
	'kernel/memorymanager.cpp',
	'kernel/physical_manager.cpp',
	'kernel/filesystem/drives.cpp',
	'kernel/filesystem/directory.cpp',
	'kernel/filesystem/streams.cpp',
	'kernel/elf.cpp',
	'kernel/interrupt.cpp',
	'kernel/syscall.c',
	'kernel/task.cpp',
	'kernel/locks.cpp',
	'kernel/driver_loader.cpp',
	'kernel/display.cpp',
	'kernel/sysclock.cpp',
	'kernel/gdt.cpp',
	'kernel/boot_info.c',
	'kernel/rt_device.cpp',	
	'kernel/input.cpp',	
	'kernel/kassert.cpp',
	'kernel/shared_mem.cpp',

	'drivers/display/basic_text/basic_text.cpp',
	'drivers/formats/rdfs.cpp',
	'drivers/ramdisk.cpp',
	'drivers/cmos.cpp',
	'drivers/pit.cpp'
]	
clib_src = [	
	'clib/string.c',
	'clib/stdio.c',
	'clib/ctype.c',
	'clib/time.c',
	'clib/stdlib.cpp',
	'clib/liballoc.cpp',
]
clib_asm = asm_gen.process([
	'clib/string.asm'
])


common_flags = [
		'-Wuninitialized',
		'-Wall',
		'-fno-unwind-tables',
		'-fno-asynchronous-unwind-tables',
		'-march=i386',
		'-mno-sse',
		'-mno-mmx',
		'-I ./',
		'-D__I386_ONLY',
		'-O2',
		'-fomit-frame-pointer',
		'-nodefaultlibs'
	]
c_args = common_flags + [
		'-std=c99',
		'-Wc++-compat',
	]
cpp_args = common_flags + [
		'-std=c++20',
		'-fno-rtti',
		'-fno-exceptions',
	]

clib_include = ['clib/include', 'cpplib/include']

common_linker_flags = ['-Wl,--gc-sections', '-Wl,--lto-O2', '-Wl,-mllvm,-align-all-nofallthru-blocks=2', '-L' + meson.source_root()]

kernel_flags = ['-D __KERNEL', '-mno-implicit-float']
kernel_include = clib_include + ['kernel']

linker_script_deps = meson.source_root() / 'linker.ld'

rdfs = executable('rdfs', 'tools/rdfs.cpp', cpp_args: ['-std=c++20', '-O2'], native : true)
limine_install = executable('limine-install', 'tools/limine/limine-install.c', native : true)

boot_mapper = static_library('boot_mapper', 
	'kernel/boot_mapper.cpp',
	c_args: c_args + kernel_flags + ['-fno-lto'],
	cpp_args: cpp_args + ['-fno-lto'],
	include_directories: kernel_include,
	pic: true
)

executable('kernal.elf', 
	kernel_asm,
	kernel_src,
	clib_asm,
	clib_src,
	include_directories: kernel_include,
	c_args: c_args + kernel_flags + ['-flto'],
	cpp_args: cpp_args + kernel_flags + ['-flto'],
	link_args: ['-Tlinker.ld'] + common_linker_flags + ['-Wl,-N','-Wl,-z,notext', '-Wl,-Ttext=0xF000'],
	link_depends: linker_script_deps, 
	link_with: boot_mapper
)

driver_linker_script_flags = common_linker_flags + ['-s', '-Tdrivers/driver.ld']
driver_linker_script_deps = meson.source_root() / 'drivers' / 'driver.ld'

lib_emu = static_library('x86emu', 
	[
		'drivers/display/vesa/libx86emu/api.c',
		'drivers/display/vesa/libx86emu/decode.c',
		'drivers/display/vesa/libx86emu/mem.c',
		'drivers/display/vesa/libx86emu/ops.c',
		'drivers/display/vesa/libx86emu/ops2.c',
		'drivers/display/vesa/libx86emu/prim_ops.c'
	],
	c_args: c_args + kernel_flags + ['-flto', '-fvisibility=hidden', '-std=gnu99'],
	include_directories: kernel_include + ['drivers/display/vesa/'],
	pic: true
)

driver_map = {}
drivers = [
		['AT_kbrd', ['drivers/at_kbrd.cpp'], []],
        ['isa_dma', ['drivers/isa_dma.cpp'], []],
		['drvlib', ['drivers/drvlib.cpp'], []],
		['pci', ['drivers/pci.cpp'], ['drvlib']],
		['floppy', ['drivers/floppy.cpp'], ['drvlib', 'isa_dma']],
		['vga', ['drivers/display/vga/vga.cpp'], ['drvlib']],
		['vesa', ['drivers/display/vesa/vesa.cpp'], ['drvlib'], lib_emu],
		['mbr', ['drivers/formats/mbr.cpp'], ['drvlib']],
		['ata', ['drivers/ata.cpp'], ['drvlib', 'pci']],
		['fat', ['drivers/formats/fat.cpp'], ['drvlib']],
		['iso9660', ['drivers/formats/iso9660.cpp'], ['drvlib']],
		['i8042', ['drivers/i8042.cpp'], ['drvlib']],
		['ps2mouse', ['drivers/ps2mouse.cpp'], ['drvlib']],
		]

foreach d : drivers

deps = []
foreach dep : d[2]
	deps += driver_map.get(dep)
endforeach

drv = shared_library(d[0], d[1],
	c_args: c_args + kernel_flags + ['-flto'],
	cpp_args: cpp_args + kernel_flags + ['-flto'],
	include_directories: kernel_include,
	link_args: driver_linker_script_flags,
	link_depends: driver_linker_script_deps,
	link_with: deps + d.get(3, []),
	name_prefix: '',
	name_suffix: 'drv',
	override_options: 'b_lundef=false'
)

driver_map += {d[0] : drv}

endforeach

cpp_runtime = static_library('cppruntime', 
	['api/cppruntime.cpp'],
	cpp_args: cpp_args + ['-flto'],
	include_directories: clib_include + ['api/'],
	pic: true
)

kbrd = static_library('keyboard', 
	['api/keyboard.c'],
	c_args: c_args + ['-flto'],
	include_directories: clib_include + ['api/'],
	pic: true
)

clib = shared_library('clib',	
	clib_asm,
	clib_src,
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	include_directories: clib_include + ['api/'],
	link_args: ['-s'] + common_linker_flags,
	name_prefix: '',
	name_suffix: 'lib',
	override_options: 'b_lundef=false'
)

terminal = shared_library('terminal',	
	['api/terminal/terminal.cpp', 'api/cppruntime.cpp'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	include_directories: clib_include + ['api/'],
	link_args: ['-s'] + common_linker_flags,
	name_prefix: '',
	name_suffix: 'lib',
	override_options: 'b_lundef=false'
)

crti = asm_gen.process(['api/crti.asm'])
crtn = asm_gen.process(['api/crtn.asm'])

executable('shell.elf', 
	crti, ['api/crt0.c', 'shell/commands.cpp', 'shell/shell.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

executable('primes.elf', 
	crti, ['api/crt0.c', 'apps/primes.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

executable('bkgrnd.elf', 
	crti, ['api/crt0.c', 'apps/bkgrnd.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

executable('listmode.elf', 
	crti, ['api/crt0.c', 'apps/listmode.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

executable('graphics.elf', 
	crti, ['api/crt0.c', 'apps/graphics.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

executable('fwrite.elf', 
	crti, ['api/crt0.c', 'apps/fwrite.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, cpp_runtime]
)