[bits 16]
%assign root_size 		(32 * _root_entries) / _bytes_per_sector
%assign fats_size		(_number_of_FATs * _sectors_per_FAT)
%assign root_location 	fats_size + _reserved_sectors
%assign datasector  	root_location + root_size
%assign cluster_size	_sectors_per_cluster * _bytes_per_sector

FAT_LOCATION equ 0x500
ROOT equ FAT_LOCATION + _bytes_per_sector
DIR_ENTRY_SIZE equ 0x0020

; es:bx = load address, si = file name
; trashes ax, dx, si, cx, di
fat_load : 	
	; read root directory into memory (0x0500)
		push	bx								; save load address
		
		mov     cx, root_size    					
		mov  	ax, root_location            		
		mov   	bx, ROOT
		call  	load_disk_ss

		pop		bx								; restore load address
	; browse root directory for filename
		mov		ax, _root_entries				; load loop counter
		mov		di, ROOT                        ; locate first root entry

	.LOOP:
		push	es
		push	ds
		pop		es
		push	si
		push	di
		mov		cx, 0x000B						; eleven character name
		repe  	cmpsb							; test for entry match
		pop		di
		pop		si
		pop		es
		je		LOAD_FAT
		add		di, DIR_ENTRY_SIZE				; queue next directory entry
		dec		ax
		jnz		.LOOP
FAILURE:
		jmp $									; did not find kernel image

LOAD_FAT:
		xor ax, ax
		;mov [fat_sector], ax ;reset the fat sector to 0
		mov     dx, WORD [di + 0x001A]			; get the first cluster from the directory entry
		
	LOAD_FILE:
		mov     ax, dx								; cluster to read
	push	ax									; save cluster on stack

	; LBA = (cluster - 2) * _sectors_per_cluster + datasector
	; convert cluster to LBA
		sub     ax, 0x0002                          ; zero base cluster number
		mov     cx, _sectors_per_cluster   			; convert byte to word
		mul     cx
		add     ax, datasector						; base data sector
		
		call  	disk_load							; load cluster to es:bx
		
	; compute next cluster
	pop     si									; restore cluster from stack
		mov     ax, si								; cluster + cluster / 2 
		shr     ax, 0x0001                          ; divide by two
		add     ax, si

	push 	bx									; save load address

	; reads a word from the fat at index ax into dx
		xor		dx,	dx
		div		word [bp_bytes_per_sector]
		add     ax, _reserved_sectors 
		cmp		ax, [fat_sector]
	push	dx							; save the offset into the fat sector
		je		.skip_fat_load	

	; read FAT into memory (0x0500)
		inc		cx				;1 sector, we already know cx == 0 after disk_load
		mov		[fat_sector], ax	;save the fat sector
		mov     bx, FAT_LOCATION	;read the FAT to ss:0x0500
		call  	load_disk_ss 		;load	
	.skip_fat_load:
	pop		bx							; restore the offset into the fat sector
		mov     dx, WORD [bx + FAT_LOCATION]

		test    si, 0x0001
		jz     .EVEN_CLUSTER
	.ODD_CLUSTER:
		shr     dx, 0x0004                          ; take high twelve bits
	.EVEN_CLUSTER:
		and     dx, 0x0FFF							; take low twelve bits

	.DONE:     
	pop 	bx										; restore load address
		cmp     dx, 0x0FF0                          ; test for end of file
		jb      LOAD_FILE
		
	DONE:
		ret											; file is loaded

load_disk_ss:
	push	es
	push	ss
	pop		es
	call  	disk_load 			;load	
	pop		es
	ret
	
fat_sector	dw 0