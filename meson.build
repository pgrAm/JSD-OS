project('jsd-os', ['c', 'cpp'])

nasm = find_program('nasm', required: true)

asm_gen = generator(nasm,
  output : '@BASENAME@.o',
  arguments : [
    '-f elf',
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


executable('kernal.elf', kernel_asm, kernel_src, clib_asm, clib_src, include_directories: ['kernel', 'clib/include', 'cpplib/include'] )

