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
	;mov dword [a20_enable], 1
	call deal_with_a20
	A20_on:
	
	call loadGDT
	
	mov ax, GDT_DATA_SEG
	mov ds, ax
	mov ss, ax
	mov es, ax 
	mov fs, ax
	mov gs, ax
	
runkernel:		
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

;	mov ebp, 0x90000 ; Update our stack position so it is right at the top of the free space.
;	mov esp, ebp 
;	jmp runkernel

; GDT
gdt_start :
gdt_null :
dq 0x0000000000000000
gdt_code : 	
dw 0xffff 	
dw 0x0 
db 0x0 		
db 10011010b
db 11001111b
db 0x0 		
gdt_data : 	
dw 0xffff 	
dw 0x0 		
db 0x0		
db 10010010b
db 11001111b
db 0x0 		
gdt_end :
 	
gdt_descriptor :
dw gdt_end - gdt_start - 1
dd gdt_start

GDT_CODE_SEG equ gdt_code - gdt_start
GDT_DATA_SEG equ gdt_data - gdt_start