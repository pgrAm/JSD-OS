#include "interrupt.h"
#include "syscall.h"
#include "filesystem.h"
#include "task.h"
#include "memorymanager.h"
#include "../drivers/sysclock.h"
#include "../drivers/kbrd.h"
#include "../drivers/video.h"
//A syscall is accomplished by
//putting the arguments into EAX, ECX, EDX, EDI, ESI
//put the system call index into EBX
//int 0x80
//EAX has the return value

SYSCALL_HANDLER int _empty() 
{ 
	return 0;
}

extern void handle_syscall();

const void* syscall_table[] =
{
	filesystem_find_file_by_path,
	filesystem_open_file,
	filesystem_close_file,
	filesystem_read_file,
	exit_process,
	spawn_process, 
	sysclock_get_master_time,
	sysclock_get_ticks,
	sysclock_get_utc_offset,
	memmanager_virtual_alloc,
	memmanager_free_pages,
	get_keypress,
	wait_and_get_keypress,
	filesystem_open_file_handle,
	filesystem_open_directory_handle,
	filesystem_get_file_in_dir,
	filesystem_get_file_info,
	filesystem_get_root_directory,
	set_video_mode,
	map_video_memory,
	filesystem_open_directory,
	filesystem_close_directory,
	set_cursor_offset,
	get_keystate
};

const size_t num_syscalls = sizeof(syscall_table) / sizeof(void*);

void setup_syscalls()
{
	idt_install_handler(0x80, handle_syscall, IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
}