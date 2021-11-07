[bits 16]

getmem:						;low memory in KB is stored in ax, upper memory in bx and 32 bit memory in cx in blocks of 64k
	getuppermemoryxe801:
		;xor dx, dx		;set dx to 0
		xor cx, cx		;set cx to 0
	
		mov ax, 0xe801 	;set up the ax for the BIOS call	
		call int15_check_valid
		jcxz .useax		;if cx is 0 then try ax
		mov ax, cx		;ax contains the number of KB between 1 and 16 MB
		;mov cx, dx		;bx contains the number of 64k blocks between 16 MB and 4 GB
	.useax:
		cmp ax, 0x3c00	;sanity check, we shouldn't be getting more than 15 MB
		jg 	getuppermemoryx88	;jump if not sane
		test ax, ax
		jnz done_high	;raise an error if no memory is returned
		
	getuppermemoryx88:
		clc
		mov ax, 0x8800	;set up the ax for the BIOS call
		call int15_check_valid

done_high:

	mov word [MULTIBOOT_OFFSET + multiboot_info.memoryHi], ax
	
	getlowmemory:
		clc
		int 0x12		;call BIOS interrupt
		jnc .finish		;handle general error
		xor ax, ax		;signal an error by getting no memoru
	.finish:	
		mov word [MULTIBOOT_OFFSET + multiboot_info.memoryLo], ax
		ret

;calls int 15 then checks the result for validity
int15_check_valid:
	int 0x15
	jc .memerror	;handle general error
	cmp ah, 0x86
	je .memerror	;raise an error if the BIOS doesn't support the function
	cmp ah, 0x80
	je .memerror	;raise an error if the our command was invalid	
	ret
.memerror:
	xor ax, ax		;return 0 to let us know the shit hit the fan
	ret