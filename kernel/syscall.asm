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

struc FRAME
	.eip0:	resd 1
    .gs:	resd 1 
	.fs:	resd 1 
	.es:	resd 1
	.ds:	resd 1
	.eip:	resd 1 
	.cs:	resd 1
	.eflags:resd 1
	.esp:	resd 1
	.ss:	resd 1
endstruc

global __regcall3__iopl

__regcall3__iopl:
	mov ecx, [esp + FRAME.eflags]
	and ecx, 0xFFFFCFFF
	shl eax, 12
	and eax, 0x00003000
	or	ecx, eax
	mov [esp + FRAME.eflags], ecx
	xor eax, eax
	ret
