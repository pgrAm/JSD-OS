; Ensures that we jump straight into the kernel ’s entry function.
[bits 32] 		; We’re in protected mode by now , so use 32 - bit instructions.
[extern kernel_main] 	; Declare that we will be referencing the external symbol 'main ’,
[extern _BSS_END_]
global _multiboot

start:
	jmp header_end
	
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
	dd _BSS_END_		;no bss

    ; required end tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot2_header_end:
	align 4
multiboot1_header:
    dd 0x1BADB002
	dd 0x00010006			; bit 16 & 2 & 1 set
	dd -(0x1BADB002 + 0x00010006)
	dd multiboot1_header	; header_address
	dd start				; load_address
	dd 0x00000000			; load_end_address
	dd _BSS_END_			; bss_end_address
	dd start				; entry_address
	dd 1 ; text mode
	dd 1 ; text mode
	dd 80
	dd 25
	dd 0
multiboot1_header_end:
header_end:
	cli
	mov ebp, 0x00090000 ; Update our stack position so it is right at the top of the free space.
	mov esp, ebp

	; set gdt
	lgdt [gdt_descriptor]
	jmp GDT_CODE_SEG : set_regs
set_regs:
	mov ax, GDT_DATA_SEG
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov dword [_multiboot], ebx

	pushad
	mov edi, 0x112345 ;odd megabyte address.
	mov esi, 0x012345 ;even megabyte address.
	mov [esi], esi    ;making sure that both addresses contain different values.
	mov [edi], edi    ;(if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
	cmpsd             ;compare addresses to see if the're equivalent.
	popad
	jne A20_on
	call deal_with_a20
	A20_on:

	call loadTSS

	call kernel_main	; invoke main () in our C kernel
	jmp $ 				; Hang forever when we return from the kernel

_multiboot dd 0

deal_with_a20:
    call    a20wait
    mov     al, 0xAD
    out     0x64, al
    call    a20wait
    mov     al, 0xD0
	out     0x64, al
    call    a20wait2
    in      al, 0x60
    push    eax
    call    a20wait
    mov     al, 0xD1
    out     0x64, al
    call    a20wait
    pop     eax
    or      al, 2
    out     0x60, al
    call    a20wait
    mov     al, 0xAE
    out     0x64, al
    call    a20wait
    ret
a20wait:
    in      al, 0x64
    test    al, 2
    jnz     a20wait
    ret
a20wait2:
    in      al, 0x64
    test    al, 1
    jz      a20wait2
    ret

loadTSS:
	mov	ax, GDT_TSS_SEG | 3
	ltr ax
	ret
	
; GDT
gdt_start :
gdt_null :
dq 0x0000000000000000
gdt_code : 	
dw 0xffff 	
dw 0x0000 
db 0x00 		
db 0x9A
db 0xCF ;flags and top nibble of limit
db 0x00 		
gdt_data : 	
dw 0xffff 	
dw 0x0000 		
db 0x00		
db 0x92
db 0xCF ;flags and top nibble of limit
db 0x00
gdt_usr_code : 	
dw 0xffff 	
dw 0x0000 
db 0x00 		
db 0xFA
db 0xCF ;flags and top nibble of limit
db 0x00 		
gdt_usr_data : 	
dw 0xffff 	
dw 0x0000 		
db 0x00		
db 0xF2
db 0xCF ;flags and top nibble of limit
db 0x00
gdt_tss:
dw TSS_SIZE & 0x0000FFFF		;this is the limit for the tss
dw TSS_OFFSET & 0x0000FFFF		;this is the base for the tss
db (TSS_OFFSET >> 16) & 0xFF	;this the the high bits	fot the base
db 0x89	
db ((TSS_SIZE >> 16) & 0x0F) | 0x40	;= ((limit & 0xF0000) >> 16) | 0x40
db 0x00 				
gdt_end :
 	
gdt_descriptor :
dw gdt_end - gdt_start - 1
dd gdt_start

GDT_CODE_SEG equ gdt_code - gdt_start
GDT_DATA_SEG equ gdt_data - gdt_start
GDT_USER_CODE_SEG equ gdt_usr_code - gdt_start
GDT_USER_DATA_SEG equ gdt_usr_data - gdt_start
GDT_TSS_SEG equ gdt_tss - gdt_start


global tss_esp0_location
tss_begin:
	dd 	0x00000000
tss_esp0_location:
	dd	0x00090000			;kernel stack pointer
	dd	GDT_DATA_SEG		;kernel stack segment
	times 23 dd 0x00000000
tss_end:

TSS_SIZE equ (tss_end - tss_begin)
TSS_OFFSET equ 0x8000 + tss_begin - $$

global run_user_code
run_user_code:
	mov ebx, [esp + 4] ;code address
	mov ecx, [esp + 8] ;stack address

	mov ax,	GDT_USER_DATA_SEG | 3
	mov ds,	ax
	mov es,	ax 
	mov fs,	ax 
	mov gs,	ax ;we don't need to worry about SS. it's handled by iret
	
	push GDT_USER_DATA_SEG | 3 	;user data segment with bottom 2 bits set for ring 3
	push ecx 					;push the new user stack
	pushf						;push flags
	push GDT_USER_CODE_SEG | 3	;user code segment with bottom 2 bits set for ring 3
	push ebx 					;address of the user function
	iret

global current_task_TCB
current_task_TCB dd 0

struc TCB
    .esp:	resd 1
    .esp0:	resd 1
    .cr3:	resd 1
endstruc
	
global switch_task
switch_task:
	pushfd
	cli
    push ebx
    push esi
    push edi
    push ebp

    mov edi, [current_task_TCB]		;edi = address of the previous task's "thread control block"
   	mov eax, cr3
	mov [edi + TCB.esp], esp		;Save ESP for previous task's kernel stack in the thread's TCB
	mov [edi + TCB.cr3], eax

    ;Load next task's state
    mov esi, [esp + (5+1)*4]		;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task_TCB], esi		;Current task's TCB is the next task TCB

    mov esp, [esi + TCB.esp]		;Load ESP for next task's kernel stack from the thread's TCB
    mov eax, [esi + TCB.cr3]		;eax = address of page directory for next task
    mov ebx, [esi + TCB.esp0]		;ebx = address for the top of the next task's kernel stack
    mov [tss_esp0_location], ebx	;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov ecx, cr3					;ecx = previous task's virtual address space

    cmp eax, ecx					;Does the virtual address space need to being changed?
    je .doneVAS						;no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3, eax					;yes, load the next task's virtual address space
.doneVAS:
    pop ebp
    pop edi
    pop esi
    pop ebx
	popfd
    ret
	