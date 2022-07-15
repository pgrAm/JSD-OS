[bits 32]

section .data

struc GDT_SEG
    .limit_lo:		resw 1
    .base_lo:		resw 1
    .base_mid:		resb 1
	.access:		resb 1
	.granularity:	resb 1
	.base_hi:		resb 1
endstruc

struc GDT
	.null:		resb GDT_SEG_size
	.code:		resb GDT_SEG_size
	.data:		resb GDT_SEG_size
	.user_code: resb GDT_SEG_size
	.user_data: resb GDT_SEG_size
	.tls_data:	resb GDT_SEG_size
	.cpu_data:	resb GDT_SEG_size
	.tss_data:	resb GDT_SEG_size
endstruc

struc TSS
	.link:		resw 1
	.reserved0:	resw 1
	.esp0:		resd 1
	.ss:		resw 1
	.reserved1:	resw 1
	.other:		resd 23
endstruc

struc CPU_state
	.self_ptr:		resd 1

	.tss:	resb TSS_size
	.gdt:	resb GDT_size

	.tcb_ptr: resd 1
endstruc

struc TCB
    .esp:			resd 1
    .esp0:			resd 1
    .cr3:			resd 1
	.tls_gdt_hi:	resd 1
	.tls_base_lo:	resw 1
	.padding:		resw 1

	.running:		resb 1
endstruc

section .text

global run_user_code
run_user_code:
	mov ebx, [esp + 4] ;code address
	mov ecx, [esp + 8] ;stack address

	mov ax,	GDT.user_data
	mov ds,	ax
	mov es,	ax
	mov fs, ax

	;FS can be left as is (CPU seg)		
	;GS can be left as is (TLS seg)		
	
	;SS & CS set up here for iret to hande:	

	push GDT.user_data | 3 	;user data segment with bottom 2 bits set for ring 3
	push ecx 				;push the new user stack
	pushf					;push flags
	push GDT.user_code | 3	;user code segment with bottom 2 bits set for ring 3
	push ebx 				;address of the user function
	iret

[extern current_task_TCB] 

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

    mov edi, [fs:CPU_state.tcb_ptr]		; edi = address of the previous task's "thread control block"
   	mov eax, cr3
	mov [edi + TCB.esp], esp		; Save ESP for previous task's kernel stack in the thread's TCB
	mov [edi + TCB.cr3], eax

	;Load next task's state
    mov esi, [esp + (5+1)*4]		; esi = address of the next task's "thread control block" (parameter passed on stack)

	; atomically mark the previous task as not running
	; don't touch the stack after this
	xor al, al
	xchg [edi + TCB.running], al

load_new_task:
    mov [fs:CPU_state.tcb_ptr], esi		; Current task's TCB is the next task TCB

	;copy the required data into the gdt
	mov ax, [esi + TCB.tls_base_lo]
	mov [fs:(GDT.tls_data + GDT_SEG.base_lo)], ax
	mov eax, [esi + TCB.tls_gdt_hi]
	mov [fs:(GDT.tls_data + GDT_SEG.base_mid)], eax

	;reload gs
	mov ax, GDT.tls_data | 3
	mov gs,	ax

    mov esp, [esi + TCB.esp]			; Load ESP for next task's kernel stack from the thread's TCB
    mov eax, [esi + TCB.cr3]			; eax = address of page directory for next task
    mov ebx, [esi + TCB.esp0]			; ebx = address for the top of the next task's kernel stack
	
	mov [fs:(CPU_state.tss + TSS.esp0)], ebx	; Adjust the ESP0 field in the TSS

	; check if we need to change page tables
    mov ecx, cr3
    cmp eax, ecx						
    je .skip_cr3							
    mov cr3, eax						
.skip_cr3:
    pop ebp
    pop edi
    pop esi
    pop ebx
	popfd
    ret