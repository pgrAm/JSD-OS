; Ensures that we jump straight into the kernel ’s entry function.
[bits 32] 		; We’re in protected mode by now , so use 32 - bit instructions.
[extern kernel_main] 	; Declare that we will be referencing the external symbol 'main ’,
[extern boot_remap_addresses]
[extern _BSS_END_]
[extern _IMAGE_END_]
[extern _DATA_END_]
[extern init_stack]
[extern gdt_location]
[extern gdt_descriptor_location]

global _boot_eax
global _boot_ebx
global _kernel_location

GDT_CODE_SEG equ 0x08 ; gdt_data_location - gdt_location
GDT_DATA_SEG equ 0x10 ; gdt_code_location - gdt_location

section .boot_entry_code
global _KERNEL_START_
_KERNEL_START_:
	cli
	push eax
	push ebx
	call get_ip
get_ip:
	pop		ebx
	;compute the real address of _KERNEL_START_
	sub		ebx, (get_ip - _KERNEL_START_) 
	
	push	ebx
	push	(_IMAGE_END_ - _KERNEL_START_)
	push	_KERNEL_START_

	; call a C++ function which sets up page tables
	add		ebx, (boot_remap_addresses - _KERNEL_START_)
	call	ebx

	; set cr3 to value returned by boot_remap_addresses
	mov		cr3, eax 

	add		esp, 8
	pop		ebx

	pop		eax ;contains boot ebx
	mov		dword [ebx + (_boot_ebx - _KERNEL_START_)], eax
	pop		eax
	mov		dword [ebx + (_boot_eax - _KERNEL_START_)], eax
	mov		dword [ebx + (_kernel_location - _KERNEL_START_)], ebx

	mov ebp, init_stack + 0x1000
	mov esp, ebp

	mov eax, pf_handler
	mov [ebx + (pf_handler_lo_addr - _KERNEL_START_)], ax
	shr eax, 0x10
	mov [ebx + (pf_handler_hi_addr - _KERNEL_START_)], ax

	;mov ax, cs
	;mov [ebx + (pf_handler_seg - _KERNEL_START_)], ax

	lgdt [ebx + (gdt_descriptor_location - _KERNEL_START_)]
	
	lidt [ebx + (idtr - _KERNEL_START_)]

	; set up out paging values
	mov eax, cr0
	or  eax, 0x80000001 
	mov cr0, eax			; enable paging, the next instruction will page fault
	ud2

pf_handler:
	mov ax, GDT_DATA_SEG
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov byte [pf_handler_flags], 0
	mov dword [esp+4], GDT_CODE_SEG
	mov dword [esp], begin_in_VM
	iret

begin_in_VM:
	mov ebp, init_stack + 0x1000
	mov esp, ebp

	call kernel_main	; invoke main () in our C kernel
	jmp $ 				; Hang forever when we return from the kernel
	
header_start:
	align 8
multiboot2_header:
    dd 0xe85250d6                ; magic number (multiboot 2)
    dd 0                         ; architecture 0 (protected mode i386)
    dd multiboot2_header_end - multiboot2_header ; header length
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header))

    ; optional multiboot tags
	dw 2				;address tag
	dw 0
	dd 24
	dd multiboot2_header
	dd -1				;load file from start
	dd 0				;load entire file
	dd _BSS_END_

	dw 3
	dw 0
	dd 12
	dd (multiboot2_header - _KERNEL_START_) + 0x10000	; min address

	;relocation tag
	;dw 10
	;dw 0
	;dd 24
	;dd 0x7000	; min address
	;dd 0xFFFFFF ; max address 16 MB
	;dd 0x1000	; page align
	;dd 1		; load as low as possible

    ; required end tag
    ;dw 0    ; type
    ;dw 0    ; flags
   ; dd 8    ; size
multiboot2_header_end:
	align 4
multiboot1_header:
    dd 0x1BADB002
	dd 0x00010006										; bit 16 & 2 & 1 set
	dd -(0x1BADB002 + 0x00010006)
	dd 0x200000 + (multiboot1_header - _KERNEL_START_)	; header_address
	dd 0x200000 ;start				; load_address
	dd 0x200000 + (_DATA_END_ - _KERNEL_START_)			; load_end_address
	dd 0x200000 + (_IMAGE_END_ - _KERNEL_START_)		; bss_end_address
	dd 0x200000 ;start									; entry_address
	dd 1 ; text mode
	dd 80
	dd 25
	dd 0
multiboot1_header_end:
header_end:

	;call adjust_gdt

	;pushad
	;mov edi, 0x112345 ;odd megabyte address.
	;mov esi, 0x012345 ;even megabyte address.
	;mov [esi], esi    ;making sure that both addresses contain different values.
	;mov [edi], edi    ;(if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
	;cmpsd             ;compare addresses to see if the're equivalent.
	;popad
	;jne A20_on
	;call deal_with_a20
	;A20_on:



section .reclaimable_bss

_boot_eax dd 0
_boot_ebx dd 0
_kernel_location dd 0

section .reclaimable_data

temp_idt:	

times 14 dq 0

pf_handler_lo_addr:
	dw 0
pf_handler_seg:
	dw GDT_CODE_SEG
	db 0
pf_handler_flags:
	db 0x8e
pf_handler_hi_addr:
	dw 0
temp_idt_end:

idtr:
	dw (temp_idt_end - temp_idt) - 1
	dd temp_idt