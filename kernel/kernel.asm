; Ensures that we jump straight into the kernel ’s entry function.
[bits 32] 		; We’re in protected mode by now , so use 32 - bit instructions.
[extern kernel_main] 	; Declare that we will be referencing the external symbol 'main ’,
global _multiboot

start:
	cli
	push	ebp
	mov		ebp, esp
	mov		eax, [ebp + 8]
	mov 	[_multiboot], eax		

	pushad
	mov edi,0x112345  ;odd megabyte address.
	mov esi,0x012345  ;even megabyte address.
	mov [esi],esi     ;making sure that both addresses contain diffrent values.
	mov [edi],edi     ;(if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
	cmpsd             ;compare addresses to see if the're equivalent.
	popad
	jne A20_on
	call deal_with_a20
	A20_on:
	
	call loadGDT
	
	mov ax, GDT_DATA_SEG
	mov ds, ax
	mov ss, ax
	mov es, ax 
	mov fs, ax
	mov gs, ax
	
	call loadTSS
	
runkernel:	
	mov ebp, 0x90000 ; Update our stack position so it is right at the top of the free space.
	mov esp, ebp 
	
	call kernel_main 	; invoke main () in our C kernel
	jmp $ 		; Hang forever when we return from the kernel

_multiboot dd 0
global a20_enable
;a20_enable: dd 0

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

loadGDT:
	cli
	lgdt [gdt_descriptor]
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

tss_begin:
	dd 	0x00000000
	dd	0x00090000			;kernel stack pointer
	dd	GDT_DATA_SEG		;kernel stack segment
	times 23 dd 0x00000000
tss_end:

TSS_SIZE equ (tss_end - tss_begin)
TSS_OFFSET equ 0x1000 + tss_begin - $$

user_code:
	;cli
	out dx, al
	ret

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
	push ecx 					;push our current stack just for the heck of it
	pushf
	push GDT_USER_CODE_SEG | 3	;user code segment with bottom 2 bits set for ring 3
	push ebx 					;address of the user function
	iret