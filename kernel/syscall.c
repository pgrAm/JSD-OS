#include <kernel/interrupt.h>
#include <kernel/syscall.h>
#include <kernel/filesystem.h>
#include <kernel/task.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/sysclock.h>
#include <kernel/display.h>
#include <kernel/input.h>

//A syscall is accomplished by
//putting the arguments into EAX, ECX, EDX, EDI, ESI
//put the system call index into EBX
//int 0x80
//EAX has the return value

SYSCALL_HANDLER int _empty() 
{ 
	return 0;
}

extern SYSCALL_HANDLER int iopl(int val);

extern void handle_syscall();

const void* syscall_table[] =
{
	syscall_find_file_by_path,
	syscall_open_file,
	syscall_close_file,
	syscall_read_file,
	exit_process,
	spawn_process,
	sysclock_get_master_time,
	syscall_get_ticks,
	sysclock_get_utc_offset,
	syscall_virtual_alloc,
	syscall_free_pages,
	_empty,
	_empty,
	syscall_open_file_handle,
	syscall_open_directory_handle,
	syscall_get_file_in_dir,
	syscall_get_file_info,
	syscall_get_root_directory,
	set_display_mode,
	map_display_memory,
	get_display_mode,
	syscall_close_directory,
	set_cursor_offset,
	get_keystate,
	physical_num_bytes_free,
	iopl,
	set_display_offset,
	get_input_event,
	syscall_write_file,
	syscall_dispose_file_handle
};

const size_t num_syscalls = sizeof(syscall_table) / sizeof(void*);

void setup_syscalls()
{
	isr_install_handler(0x80, handle_syscall, false);
}