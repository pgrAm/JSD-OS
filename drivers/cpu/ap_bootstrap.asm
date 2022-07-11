[bits 16]
[org 0x7000]

ap_bootstrap_begin:
	cli
	cld

	xor	ax, ax
	mov ds, ax

	lgdt [ds:flat_gdt_descriptor]
	mov eax, cr0
	or	eax, 0x01
	mov cr0, eax

	jmp 0x08 : pmode

[bits 32]
pmode:
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov ebp, stack.top
	mov esp, ebp

	mov eax, dword [page_dir]
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax

	mov eax, dword [processor_id]
	mov [esp + 4], eax
	jmp [entry_point]

stack:
	align 4
	times 1024 db 0
	.top:

flat_gdt:
    dq 0
    dd 0x0000FFFF
	dd 0x00CF9A00
    dd 0x0000FFFF
	dd 0x00CF9200
    ;dd 0x00000068
	;dd 0x00CF8900
flat_gdt_end:

flat_gdt_descriptor :
	dw flat_gdt_end - flat_gdt - 1
	dd flat_gdt

ap_bootstrap_params:
	page_dir: dd 0
	entry_point: dd 0
	processor_id: dd 0