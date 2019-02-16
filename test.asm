[bits 32]
[org 0x50000]

NUMROWS equ 25
NUMCOLS equ 80
VIDEOMEM equ 0xb8000
INPUT_STATUS_1 equ 0x03da
CRTC_INDEX equ 0x03d4

start:
	pusha
	call clear_screen
	
	mov word [paddle_X], 0
	
loopstart:
	;draw paddle
	movzx ebx, word [paddle_X]
	mov ah, 0xff
	call draw_paddle

	;mov ecx, 0xFFFF
	;.label:
	;loop .label
	
	call vertical_sync
	
	;clear paddle
	movzx ebx, word [paddle_X]
	mov ah, 0x0
	call draw_paddle
	
	inc word [paddle_X]
	cmp word [paddle_X], 80
	jl loopstart
	
	popa
	mov eax, 42
	ret

paddle_X: dw 0	
ball_X: dw 0	
ball_Y: dw 0	
ball_dirX: dw 0
ball_dirY: dw 0

clear_screen:
	mov edi, VIDEOMEM
	mov al, 0
	mov ecx, 4000
	rep stosb
	ret
	
; takes ebx as horizontal coord, ah color
draw_paddle:
	mov edi, VIDEOMEM
	add edi, NUMCOLS * 2 * (NUMROWS - 1)
	shl ebx, 1
	add edi, ebx
	mov ecx, 8
	mov al, 0
	rep stosw
	ret
	
vertical_sync:	
	xor ax, ax
	mov dx, INPUT_STATUS_1
	waitForVretBegin :
	in al, dx
	test al, 0x08
	jnz waitForVretBegin
	waitForVretEnd :
	in al, dx
	test al, 0x08
	jz waitForVretEnd
	ret