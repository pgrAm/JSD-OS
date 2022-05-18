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

boot = custom_target('boot_sect',
  output : 'boot_sect.bin',
  input : 'boot/boot_sect.asm',
  command : ['nasm', '@INPUT@', '-i', meson.source_root() / 'boot', '-f', 'bin', '-o', '@OUTPUT@'], 
  build_by_default: true)

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
		'-Wconversion',
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

subdir('tools')

boot_mapper = static_library('boot_mapper', 
	'kernel/boot_mapper.cpp',
	c_args: c_args + kernel_flags + ['-fno-lto'],
	cpp_args: cpp_args + ['-fno-lto'],
	include_directories: kernel_include,
	pic: true
)

kernal = executable('kernal.elf', 
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

kernal_sys = custom_target('kernal',
	output : 'kernal.bin',
	input : kernal,
	command : [
		'objcopy', 
		'-x', 
		'-S', '-O', 'binary', 
		'@INPUT@', '@OUTPUT@',
		'--set-section-flags', '.bss=alloc,load,contents',
		'--gap-fill', '0'
	], 
	build_by_default: true)

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
	c_args: c_args + kernel_flags + ['-Wno-conversion', '-flto', '-fvisibility=hidden', '-std=gnu99'],
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

shell = executable('shell.elf', 
	crti, ['api/crt0.c', 'shell/commands.cpp', 'shell/shell.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

primes = executable('primes.elf', 
	crti, ['api/crt0.c', 'apps/primes.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

bkgrndtest = executable('bkgrnd.elf', 
	crti, ['api/crt0.c', 'apps/bkgrnd.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

listmode = executable('listmode.elf', 
	crti, ['api/crt0.c', 'apps/listmode.cpp'], crtn,
	include_directories: clib_include + ['api/'],
	c_args: c_args + ['-flto'],
	cpp_args: cpp_args + ['-flto'],
	link_args: ['-s', '-Wl,--image-base=0x8000000'] + common_linker_flags,
	link_with: [clib, terminal, kbrd, cpp_runtime]
)

graphics = executable('graphics.elf', 
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

rdfs_command = [rdfs, '@INPUT@', '-o', '@OUTPUT@']

subdir('configs/cdboot')
subdir('configs/fdboot')
subdir('configs/netboot')

fs = import('fs')

iso_contents = [
	['LICENSE.txt', meson.source_root() / 'LICENSE.txt'],
	['cdboot.sys', meson.source_root() / 'configs/cdboot/cdboot.sys'],
	[graphics.name(), graphics],
	[fs.name(clib.path()), clib],
	[fs.name(terminal.path()), terminal],
	[shell.name(), shell],
	[primes.name(), primes],
	[bkgrndtest.name(), bkgrndtest],
	['drivers/' + fs.name(driver_map.get('fat').path()), driver_map.get('fat')],
	['drivers/' + fs.name(driver_map.get('floppy').path()), driver_map.get('floppy')],
	['drivers/' + fs.name(driver_map.get('isa_dma').path()), driver_map.get('isa_dma')],
	['drivers/' + fs.name(driver_map.get('vga').path()), driver_map.get('vga')],
	['drivers/' + fs.name(driver_map.get('vesa').path()), driver_map.get('vesa')],
	['drivers/' + fs.name(driver_map.get('mbr').path()), driver_map.get('mbr')],
	['drivers/' + fs.name(driver_map.get('i8042').path()), driver_map.get('i8042')],
	['drivers/' + fs.name(driver_map.get('ps2mouse').path()), driver_map.get('ps2mouse')],
	['drivers/font08.psf', meson.source_root() / 'fonts/font08.psf'],
	['drivers/font16.psf', meson.source_root() / 'fonts/font16.psf'],
#] + [
	['boot/limine.cfg', meson.source_root() / 'limine.cfg'],
	['system/init.rfs', cdrfs],
	['system/kernal.sys', kernal_sys]
]

iso_files = []
iso_cmd = []
iso_index = 0
foreach file : iso_contents
	iso_files += file[1]
	iso_cmd += file[0] + '=@INPUT' + iso_index.to_string() + '@'
	iso_index += 1
endforeach

iso = custom_target('boot_iso',
  output : 'jsd-os+limine.iso',
  input : iso_files,
  command : ['perl', meson.source_root() / 'tools/buildiso.pl', '-d', '@OUTDIR@', '-o', '@OUTPUT@'] + iso_cmd, 
  build_by_default: true)