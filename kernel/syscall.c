#include "interrupt.h"

//A syscall is accomplished by
//putting the arguments into EAX, ECX, EDX, EDI, ESI
//put the system call index into EBX
//int 0x80
//EAX has the return value
__attribute__((regcall)) void syscall_print_string(const char *a)
{
	print_string(a);
}

extern void handle_syscall();

const void* syscall_table[] =
{
	syscall_print_string
};

const size_t num_syscalls = sizeof(syscall_table) / sizeof(void*);

void setup_syscalls()
{
	idt_install_handler(0x80, handle_syscall, IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
}