#include "interrupt.h"
#include "syscall.h"
#include "filesystem.h"
//A syscall is accomplished by
//putting the arguments into EAX, ECX, EDX, EDI, ESI
//put the system call index into EBX
//int 0x80
//EAX has the return value
SYSCALL_HANDLER void syscall_print_string(const char *a)
{
	print_string(a);
}

SYSCALL_HANDLER void syscall_exit(int val)
{
	enter_console();
}

extern void handle_syscall();

const void* syscall_table[] =
{
	syscall_print_string,
	filesystem_open_file,
	filesystem_close_file, 
	filesystem_read_file,
	syscall_exit
};

const size_t num_syscalls = sizeof(syscall_table) / sizeof(void*);

void setup_syscalls()
{
	idt_install_handler(0x80, handle_syscall, IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
}