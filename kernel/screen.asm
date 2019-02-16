[ bits 32]
; Define some constants
VIDEOMEM equ 0xb8000

asm_clear_screen:
	pusha
	
	mov edx, VIDEO_MEMORY
	mov al, 0x00
clear_screen_loop:	
	mov [edx], ax
	add edx, 2
	;add ah, 0x10
	cmp edx, VIDEOMEM + 4000 ;end of screen
	jle clear_screen_loop
	popa
	ret
	