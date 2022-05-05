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
		'-target', 'i386-unknown-elf',
		'-Wuninitialized',
		'-Wall',
		'-fno-unwind-tables',
		'-fno-asynchronous-unwind-tables',
		'-march=i386',
		'-mno-sse',
		'-mno-mmx',
		'-I ./',
		'-D__I386_ONLY',
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
linker_script_flags = ['-L' + meson.source_root(), '-Tlinker.ld']
linker_script_deps = meson.source_root() / 'linker.ld'
boot_mapper = static_library('boot_mapper', 
	'kernel/boot_mapper.cpp',
	c_args: c_args + ['-fPIC', '-D__KERNEL', '-mno-implicit-float'],
	cpp_args: cpp_args +  ['-fPIC', '-D__KERNEL', '-mno-implicit-float'],
	include_directories: ['kernel', 'clib/include', 'cpplib/include'])

executable('kernal.elf', 
	kernel_asm,
	kernel_src,
	clib_asm,
	clib_src,
	c_args: c_args + ['-D__KERNEL', '-mno-implicit-float'],
	cpp_args: cpp_args + ['-DTESTING', '-D__KERNEL', '-mno-implicit-float'],
	link_args: linker_script_flags + ['-Wl,-z,notext', '-Wl,--lto-O2', '-Wl,-N', '-Wl,-Ttext=0xF000'],
	include_directories: ['kernel', 'clib/include', 'cpplib/include'],
	link_depends: linker_script_deps, 
	link_with: boot_mapper
)

