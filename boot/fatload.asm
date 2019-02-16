[bits 16]
%assign root_size 		(32 * _root_entries) / _bytes_per_sector
%assign fats_size		(_number_of_FATs * _sectors_per_FAT)
%assign root_location 	fats_size + _reserved_sectors
%assign datasector  	root_location + root_size
%assign cluster_size	_sectors_per_cluster * _bytes_per_sector

ROOT equ 0x500
FAT_LOCATION equ 0x500
DIR_ENTRY_SIZE equ 0x0020

fat_load : 
	
    ; read root directory into memory (0x0500)
		mov     cx, root_size    					
		mov  	ax, root_location            		
		mov   	bx, ROOT
		call  	disk_load
		
    ; browse root directory for binary image
		mov		cx, _root_entries             		; load loop counter
		mov		di, ROOT                          ; locate first root entry
		
    .LOOP:
		push 	cx
		mov		cx, 0x000B                          ; eleven character name
		mov		si, file_name                       ; image name to find
		push	di
		rep  	cmpsb                               ; test for entry match
		pop		di
		pop		cx
		je		LOAD_FAT
		add		di, DIR_ENTRY_SIZE                  ; queue next directory entry
		loop	.LOOP
FAILURE:
		jmp $										; did not find kernel image

		
LOAD_FAT:
		
	; save starting cluster of boot image
		mov     dx, WORD [di + 0x001A]
		mov     WORD [cluster], dx                  ; file's first cluster
		
		push	word KERNEL_OFFSET					; push kernel destination address onto the stack
		
	LOAD_IMAGE:
		pop		bx
		mov     ax, WORD [cluster]                  ; cluster to read
		
	; convert cluster to LBA
		sub     ax, 0x0002                          ; zero base cluster number
		mov     cx, _sectors_per_cluster   			; convert byte to word
		mul     cx
		add     ax, datasector						; base data sector
		
		call  	disk_load
		push 	bx
		
	; compute next cluster
		mov     si, WORD [cluster]                  ; cluster + cluster / 2 
		mov     ax, si
		shr     ax, 0x0001                          ; divide by two
		add     ax, si
		call	read_fat							; read two bytes from FAT (0x0500)
		test    si, 0x0001
		jnz     .ODD_CLUSTER
		
	.EVEN_CLUSTER:
		and     dx, 0x0FFF							; take low twelve bits
		jmp     .DONE
		
	.ODD_CLUSTER:
		shr     dx, 0x0004                          ; take high twelve bits
		
	.DONE:     
		mov     WORD [cluster], dx                  ; store new cluster
		cmp     dx, 0x0FF0                          ; test for end of file
		jb      LOAD_IMAGE
		
	DONE:
		pop 	bx
		ret											; kernel image is loaded at KERNEL_OFFSET
	
; reads a word from the fat at index ax into dx
read_fat:
	; read FAT into memory (0x0500)
	xor		dx,	dx
	div		word [bp_bytes_per_sector]
	add     ax, _reserved_sectors 
	cmp		ax, [fat_sector]
	push	dx
	je		.skip_load	
	
	mov		cx, 1				;1 sector
	mov		[fat_sector], ax	;save the fat sector
	mov     bx, FAT_LOCATION	;read the FAT to es:0x0500
	call  	disk_load 			;load		
.skip_load:
	pop		bx
	mov     dx, WORD [bx + FAT_LOCATION]
	ret
	
fat_sector	dw 0
cluster     dw 0x0000
file_name   db "KERNAL  SYS"