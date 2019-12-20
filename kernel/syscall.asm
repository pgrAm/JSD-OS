[bits 32]
[extern num_syscalls] 
[extern syscall_table] 
global handle_syscall

handle_syscall:
	sti
	cmp ebx, num_syscalls
	ja .invalid_call
	push ds
	push es
	push fs
	push gs
	call [syscall_table+4*ebx]
	pop gs
	pop fs
	pop es
	pop ds
	iret
.invalid_call:
	mov eax, 0xFFFFFFFF
	o32 iret