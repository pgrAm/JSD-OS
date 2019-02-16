[bits 32]
global outb
global inb

section .text
inb:		; uint8_t inb(uint16_t)
	push	ebp
	mov		ebp, esp
	mov		edx, [ebp + 8]
	pop 	ebp
	in 		al, dx 
	ret
	
outb:		; void outb(uint16_t, uint8_t)
	push	ebp
	mov		ebp, 	esp
	mov		eax, 	[ebp + 12]
	mov		edx, 	[ebp + 8]
	out 	dx,		al
	pop		ebp
	ret
