[bits 32] 
[extern idtp]

global idt_load
idt_load:
    lidt [idtp]
    sti ;!!!!!!!!!!!!!!!!!THIS ENABLES IRQS DON'T FORGET!!!!!!!!!!!!!!!!!!!!!
	ret	
	
;make 32 new global isrs
%assign i 0
%rep 32
	global isr %+ i
%assign i i+1 
%endrep 	

%macro isr0_7 1 ;isrs 0-7 will use this definition
	isr%1:
	cli
	push byte 0
	push byte %1
	jmp isr_common_stub
%endmacro	

%assign i 0 ;define the first 8 isrs
%rep 8
	isr0_7 i
%assign i i+1 
%endrep 	

%macro isr8_31 1 ;isrs 8-31 will use this definition
	isr%1:
	cli
	push byte %1
	jmp isr_common_stub
%endmacro	

%assign i 8 ;define the next 24 isrs
%rep 24
	isr8_31 i
%assign i i+1 
%endrep 

; We call a C function in here. We need to let the assembler know
; that '_fault_handler' exists in another file
extern fault_handler

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, ss
    mov ds, ax
    mov es, ax
    ;mov fs, ax
    ;mov gs, ax
    mov eax, esp
    push eax
    call fault_handler
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
	;sti
    iret
