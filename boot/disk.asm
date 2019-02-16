[bits 16]
; load cx sectors to ES:BX from boot drive at sector ax
disk_load:
     .MAIN:
		mov		di, 20								;try at most 5 times per sector
	.LOOP_HEAD:
        push    ax
        push    cx
		
		xor     dx, dx                              ; prepare dx:ax for operation
		div     word [bp_sectors_per_track]			; calculate
		inc     dl                                  ; adjust for sector 0
		mov     cl, dl
		
		xor     dx, dx                              ; prepare dx:ax for operation
		div     word [bp_heads_per_cylinder]         ; calculate
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
		
		jnc		.SUCCESS

		xor     ax, ax                              ; BIOS reset disk;
		int     0x13
		dec 	di
		pop     cx
		pop     ax
		jnz		.LOOP_HEAD							; try reading again cause floppys sometimes fail randomly

		;xor	bh, bh
		;mov ax, 0x0E46
		;int 0x10
		
		ret											; just give up and return if we haven't got it after 5 tries
		
	.SUCCESS:
        pop     cx
        pop     ax
        add     bx, _bytes_per_sector			    ; queue next buffer
        inc     ax                                  ; queue next sector
        loop    .MAIN                               ; read next sector		
		
        ret