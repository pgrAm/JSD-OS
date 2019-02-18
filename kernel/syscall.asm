[bits 32]
[extern num_syscalls] 
[extern syscall_table] 
global handle_syscall

handle_syscall:
	cmp ebx, num_syscalls
	ja .invalid_call
	call [syscall_table+4*ebx]
	iret
.invalid_call:
	mov eax, 0xFFFFFFFF
	iret