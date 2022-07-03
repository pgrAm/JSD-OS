[bits 32]

section .data

global gdt_location
global gdt_tss_location
global gdt_tls_data
global gdt_data_location
global gdt_code_location
global tss_location
global gdt_descriptor_location

global load_TSS

load_TSS:
	mov ax, [esp + 4]
	ltr ax
	ret

; GDT
gdt_location:
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
gdt_data_location:
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
gdt_tls_data : 	
	dw 0xffff 	
	dw 0x0000 
	db 0x00 		
	db 0xF2
	db 0xCF ;flags and top nibble of limit
	db 0x00	
gdt_tss_location :
gdt_tss:
	dw 0	;this is the limit for the tss
	dw 0	;this is the base for the tss
	db 0	;this the the high bits	fot the base
	db 0	
	db 0	;= ((limit & 0xF0000) >> 16) | 0x40
	db 0				
gdt_end :

gdt_descriptor_location:
gdt_descriptor :
	dw gdt_end - gdt_start - 1
	dd gdt_start

GDT_CODE_SEG equ gdt_code - gdt_start
GDT_DATA_SEG equ gdt_data - gdt_start
GDT_USER_CODE_SEG equ gdt_usr_code - gdt_start
GDT_USER_DATA_SEG equ gdt_usr_data - gdt_start
GDT_TLS_DATA_SEG equ gdt_tls_data - gdt_start
GDT_TSS_SEG equ gdt_tss - gdt_start

section .text

global run_user_code
run_user_code:
	mov ebx, [esp + 4] ;code address
	mov ecx, [esp + 8] ;stack address

	mov ax,	GDT_USER_DATA_SEG | 3
	mov ds,	ax
	mov es,	ax 
	mov fs,	ax

	;GS can be left as is (TLS seg)
	;mov ax, GDT_TLS_DATA_SEG | 3
	;mov gs, ax					
	
	;SS & CS set up here for iret to hande:	

	push GDT_USER_DATA_SEG | 3 	;user data segment with bottom 2 bits set for ring 3
	push ecx 					;push the new user stack
	pushf						;push flags
	push GDT_USER_CODE_SEG | 3	;user code segment with bottom 2 bits set for ring 3
	push ebx 					;address of the user function
	iret

[extern current_task_TCB] 

struc GDT_SEG
    .limit_lo:		resw 1
    .base_lo:		resw 1
    .base_mid:		resb 1
	.access:		resb 1
	.granularity:	resb 1
	.base_hi:		resb 1
endstruc

struc TCB
    .esp:			resd 1
    .esp0:			resd 1
    .cr3:			resd 1
	.tss_ptr:		resd 1
	.tls_gdt_hi:	resd 1
	.tls_base_lo:	resw 1
endstruc

global switch_task_no_return
switch_task_no_return:
	cli
	mov esi, [esp + 4]
	jmp load_new_task
	
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

load_new_task:
    mov [current_task_TCB], esi		;Current task's TCB is the next task TCB

	;copy the required data into the gdt
	mov ax, [esi + TCB.tls_base_lo]
	mov [gdt_tls_data + GDT_SEG.base_lo], ax
	mov eax, [esi + TCB.tls_gdt_hi]
	mov [gdt_tls_data + GDT_SEG.base_mid], eax

	;reload gs
	mov ax, GDT_TLS_DATA_SEG | 3
	mov gs,	ax

    mov esp, [esi + TCB.esp]		;Load ESP for next task's kernel stack from the thread's TCB
    mov eax, [esi + TCB.cr3]		;eax = address of page directory for next task
    mov ebx, [esi + TCB.esp0]		;ebx = address for the top of the next task's kernel stack
	mov edi, [esi + TCB.tss_ptr]

	mov [edi+4], ebx				;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
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