[bits 16]
; load cx sectors to ES:BX from boot drive at sector ax
; trashes di, dx
; next sector at es:bx
disk_load:
	 .MAIN:
		mov		di, 20								;try at most 5 times per sector
	.LOOP_HEAD:
		push    ax									; save sector #
		push    cx									; save # of sectors
		
		xor     dx, dx                              ; prepare dx:ax for operation
		div     word [bp_sectors_per_track]			; calculate
		inc     dl                                  ; adjust for sector 0
		mov     cl, dl
		
		xor     dx, dx                              ; prepare dx:ax for operation
		div     word [bp_heads_per_cylinder]		; calculate
		mov     dh, dl
		
	; the track/cylinder number is a 10 bit value taken from the 2 high 
	; order bits of CL and the 8 bits in CH (low order 8 bits of track)
		mov     ch, al
		shl		ah, 6								; shift the lowest bits of ah to the top
		and		cl, 00111111b						; mask off the highest bits of cl
		or		cl, ah								; and them together

		mov     ax, 0x0201                            ; BIOS read sector, 1 sector

		mov     dl, BYTE [boot_info + multiboot_info.bootDevice]            ; drive
		int     0x13														; call bios interrupt
		
		pop     cx									; restore # of sectors
		jnc		.SUCCESS

		xor     ax, ax                              ; BIOS reset disk;
		int     0x13
		dec 	di
		pop     ax									; restore sector #
		jnz		.LOOP_HEAD							; try reading again cause floppys sometimes fail randomly
		
		ret											; just give up and return if we haven't got it after 5 tries
		
	.SUCCESS:
		add     bx, _bytes_per_sector			    ; queue next buffer
		jnc		.FINISH								; if we exceeded the segment size
		mov		ax, es
		add		ax, 0x1000							; increment the segment by 65536 bytes (4096)
		mov		es, ax
	.FINISH:
		pop     ax
		inc     ax                                  ; queue next sector
		loop    .MAIN                               ; read next sector		

		ret